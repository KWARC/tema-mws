/*

Copyright (C) 2010-2013 KWARC Group <kwarc.info>

This file is part of MathWebSearch.

MathWebSearch is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

MathWebSearch is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with MathWebSearch.  If not, see <http://www.gnu.org/licenses/>.

*/
/**
  * @brief File containing the implementation of the loadMwsHarvestFromFd
  * function
  *
  * @file loadMwsHarvestFromFd.cpp
  * @author Corneliu-Claudiu Prodescu
  * @date 30 Apr 2011
  *
  * License: GPL v3
  *
  */

#include <stdlib.h>                    // C general purpose library
#include <stdio.h>                     // C Standrad Input Output
#include <string.h>                    // C string library
#include <libxml/tree.h>               // LibXML tree headers
#include <libxml/parser.h>             // LibXML parser headers
#include <libxml/parserInternals.h>    // LibXML parser internals API
#include <libxml/threads.h>            // LibXML thread handling API
#include <sys/types.h>                 // Primitive System datatypes
#include <sys/stat.h>                  // POSIX File characteristics
#include <fcntl.h>                     // File control operations

#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "ParserTypes.hpp"             // Common Mws Parsers datatypes
#include "common/utils/DebugMacros.hpp"// Debug macros
#include "common/utils/macro_func.h"   // Macro functions

#include "loadMwsHarvestFromFd.hpp"

// Macros

#define MWSHARVEST_MAIN_NAME           "mws:harvest"
#define MWSHARVEST_EXPR_NAME           "mws:expr"
#define MWSHARVEST_EXPR_ATTR_URI_NAME  "url"
#define MWSHARVEST_EXPR_URI_DEFAULT    ""
#define MWSHARVEST_EXPR_MATH_NAME      "content"
#define MWSHARVEST_EXPR_SNIPPET_NAME   "data"

// Namespaces

using namespace std;
using namespace mws;
using namespace mws::types;

/**
  * @brief Callback function used with an xmlOutputBuffer
  *
  */
static inline int
copyToCharBufCallback(void*       vectorPtr,
                      const char* buffer,
                      int         len) {
    vector<char> * vec = (vector<char>*) vectorPtr;
    for (int i = 0; i < len; ++i) {
        vec->push_back(buffer[i]);
    }

    return len;
}


static inline
int setupCopyToStringWriter(MwsHarvest_SaxUserData* data) {
    xmlOutputBuffer* outPtr;

    data->buffer.clear();
    if ((outPtr = xmlOutputBufferCreateIO(copyToCharBufCallback,
                                          NULL,
                                          &data->buffer,
                                          NULL))
            == NULL) {
        fprintf(stderr, "Error while creating the OutputBuffer\n");
        return -1;
    } else if ((data->stringWriter = xmlNewTextWriter(outPtr))
               == NULL) {
        fprintf(stderr, "Error while creating the XML Writer\n");
        return -1;
    }

    return 0;
}

static inline
void tearDownCopyToStringWriter(MwsHarvest_SaxUserData* data) {
    xmlTextWriterEndDocument(data->stringWriter);
    xmlTextWriterFlush(data->stringWriter);
    xmlFreeTextWriter(data->stringWriter);
}

/**
  * @brief Callback function used to be used with an IO context parser
  *
  */
static inline int
fileXmlInputReadCallback(void* filePtr,
                        char* buffer,
                        int   len)
{
#ifdef TRACE_FUNC_CALLS
    LOG_TRACE_IN;
#endif

    int result;
    result = fread((void*) buffer,
                   sizeof(char),
                   (size_t) len,
                   (FILE*) filePtr);

#ifdef TRACE_FUNC_CALLS
    LOG_TRACE_OUT;
#endif
    return result;
}


/**
  * @brief Callback function used to be used with an IO context parser
  *
  */
static inline int
fileXmlInputCloseCallback(void* filePtr)
{
    return fclose((FILE*) filePtr);
}


/**
  * @brief This function is called before the SAX handler starts parsing the
  * document
  *
  * @param user_data is a structure which holds the state of the parser.
  */
static void
my_startDocument(void* user_data)
{
#ifdef TRACE_FUNC_CALLS
    LOG_TRACE_IN;
#endif

    MwsHarvest_SaxUserData* data = (MwsHarvest_SaxUserData*) user_data;

    data->unknownDepth           = 0;
    data->currentToken           = NULL;
    data->currentTokenRoot       = NULL;
    data->math                   = NULL;
    data->state                  = MWSHARVESTSTATE_DEFAULT;
    data->prevState              = MWSHARVESTSTATE_DEFAULT;
    data->errorDetected          = false;
    data->parsedExpr             = 0;
    data->warnings               = 0;
    data->exprUri                = "";

#ifdef TRACE_FUNC_CALLS
    LOG_TRACE_OUT;
#endif
}


/**
  * @brief This function is called after the SAX handler has finished parsing
  * the document
  *
  * @param user_data is a structure which holds the state of the parser.
  */
static void
my_endDocument(void* user_data)
{
#ifdef TRACE_FUNC_CALLS
    LOG_TRACE_IN;
#endif

    MwsHarvest_SaxUserData* data = (MwsHarvest_SaxUserData*) user_data;
    if (data->errorDetected)
    {
        fprintf(stderr, "Ending document due to errors\n");
    }

#ifdef TRACE_FUNC_CALLS
    LOG_TRACE_OUT;
#endif
}


/**
  * @brief This function is called when the SAX handler encounters the
  * beginning of an element.
  *
  * @param user_data is a structure which holds the state of the parser.
  * @param name      is the name of the element which triggered the callback.
  * @param attrs     is an array of attributes and values, alternatively
  *                  placed.
  */
static void
my_startElement(void*           user_data,
                const xmlChar*  name,
                const xmlChar** attrs)
{
#ifdef TRACE_FUNC_CALLS
    LOG_TRACE_IN;
#endif

    MwsHarvest_SaxUserData* data = (MwsHarvest_SaxUserData*) user_data;

    switch (data->state) {
    case MWSHARVESTSTATE_DEFAULT:
        if (strcmp(reinterpret_cast<const char*>(name),
                   MWSHARVEST_MAIN_NAME) == 0) {
            data->state = MWSHARVESTSTATE_IN_MWS_HARVEST;
            // Parsing the attributes
            while (NULL != attrs && NULL != attrs[0])
            {
                // NO ATTRIBUTES EXPECTED TODO HANDLE THEM
                attrs = &attrs[2];
            }
        } else {
            data->warnings++;
            // Saving the state
            data->prevState = data->state;
            // Going to an unkown state
            data->state = MWSHARVESTSTATE_UNKNOWN;
            data->unknownDepth = 1;
        }
        break;

    case MWSHARVESTSTATE_IN_MWS_HARVEST:
        if (strcmp(reinterpret_cast<const char*>(name),

                   MWSHARVEST_EXPR_NAME) == 0) {
            data->state = MWSHARVESTSTATE_IN_MWS_EXPR;
            // Parsing the attributes
            while (NULL != attrs && NULL != attrs[0]) {
                if (strcmp(reinterpret_cast<const char*>(attrs[0]),
                           MWSHARVEST_EXPR_ATTR_URI_NAME) == 0) {
                    data->exprUri = reinterpret_cast<const char*>(attrs[1]);
                } else {
                    // Invalid attribute
                    data->warnings++;
                    fprintf(stderr, "Unexpected attribute \"%s\"\n", attrs[0]);
                    // Setting exprUri to default
                    data->exprUri = MWSHARVEST_EXPR_URI_DEFAULT;
                }

                attrs = &attrs[2];
            }
        } else {
            data->warnings++;
            // Saving the state
            data->prevState = data->state;
            // Going to an unkown state
            data->state = MWSHARVESTSTATE_UNKNOWN;
            data->unknownDepth = 1;
        }
        break;

    case MWSHARVESTSTATE_IN_MWS_EXPR:
        if (strcmp(reinterpret_cast<const char*>(name),
                   MWSHARVEST_EXPR_MATH_NAME) == 0) {
            data->state = MWSHARVESTSTATE_IN_MWS_MATH;
        } else if (strcmp(reinterpret_cast<const char*>(name),
                          MWSHARVEST_EXPR_SNIPPET_NAME) == 0) {
            setupCopyToStringWriter(data);
            data->state = MWSHARVESTSTATE_IN_MWS_COPY;
            data->copyDepth = 1;
        } else {
            data->warnings++;
            // Saving the state
            data->prevState = data->state;
            // Going to an unkown state
            data->state = MWSHARVESTSTATE_UNKNOWN;
            data->unknownDepth = 1;
        }
        break;

    case MWSHARVESTSTATE_IN_MWS_MATH:
        if (data->currentToken != NULL) {
            data->currentToken = data->currentToken->newChildNode();
        } else {
            data->currentTokenRoot = CmmlToken::newRoot(true);
            data->currentToken = data->currentTokenRoot;
        }
        data->currentToken->setTag(reinterpret_cast<const char*>(name));
        // Adding the attributes
        while (NULL != attrs && NULL != attrs[0]) {
            string attributeName = reinterpret_cast<const char*>(attrs[0]);
            string attributeValue = reinterpret_cast<const char*>(attrs[1]);
            data->currentToken->addAttribute(attributeName, attributeValue);

            attrs = &attrs[2];
        }
        break;

    case MWSHARVESTSTATE_IN_MWS_COPY:
        data->copyDepth++;
        xmlTextWriterStartElement(data->stringWriter, name);
        while (NULL != attrs && NULL != attrs[0]) {
            xmlTextWriterWriteAttribute(data->stringWriter, attrs[0], attrs[1]);
            attrs = &attrs[2];
        }
        break;

    case MWSHARVESTSTATE_UNKNOWN:
        data->unknownDepth++;
        break;
    }

#if DEBUG
    printf("Beginning of element : %s \n", name);
#endif
#ifdef TRACE_FUNC_CALLS
    LOG_TRACE_OUT;
#endif
}


/**
  * @brief This function is called when the SAX handler encounters the
  * end of an element.
  *
  * @param user_data is a structure which holds the state of the parser.
  * @param name      is the name of the element which triggered the callback.
  */
static void
my_endElement(void*          user_data,
              const xmlChar* name)
{
#ifdef TRACE_FUNC_CALLS
    LOG_TRACE_IN;
#endif

    MwsHarvest_SaxUserData* data = (MwsHarvest_SaxUserData*) user_data;

    switch (data->state) {
    case MWSHARVESTSTATE_DEFAULT:
        fprintf(stderr, "Unexpected Default State for end element \"%s\"",
                reinterpret_cast<const char*>(name));
        assert(false);
        break;

    case MWSHARVESTSTATE_IN_MWS_HARVEST:
        data->state = MWSHARVESTSTATE_DEFAULT;
        break;

    case MWSHARVESTSTATE_IN_MWS_EXPR:
        data->state = MWSHARVESTSTATE_IN_MWS_HARVEST;

        if (data->math != NULL) {
            CrawlData crawlData;
            crawlData.expressionUri = data->exprUri;
            crawlData.data = data->data;

            // Add content math node to index
            int ret = data->indexManager->indexContentMath(data->math,
                                                           crawlData);
            if (ret != -1) {
                data->parsedExpr += ret;
            }

            delete data->math;
            data->math = NULL;
        } else {
            cerr << "[Warning: empty math element ]\n";
        }
        break;

    case MWSHARVESTSTATE_IN_MWS_MATH:
        if (data->currentToken == NULL) {
            data->state = MWSHARVESTSTATE_IN_MWS_EXPR;
        } else if (data->currentToken->isRoot()) {
            data->math = data->currentToken;
            data->currentToken = NULL;
            data->currentTokenRoot = NULL;
        } else {
            data->currentToken = data->currentToken->getParentNode();
        }
        break;

    case MWSHARVESTSTATE_IN_MWS_COPY:
        xmlTextWriterEndElement(data->stringWriter);
        data->copyDepth--;
        if (data->copyDepth == 0) {
            tearDownCopyToStringWriter(data);
            data->state = MWSHARVESTSTATE_IN_MWS_EXPR;
            data->data = string(data->buffer.data(), data->buffer.size());
        }
        break;

    case MWSHARVESTSTATE_UNKNOWN:
        data->unknownDepth--;
        if (data->unknownDepth == 0)
            data->state = data->prevState;
        break;
    }

#if DEBUG
    printf("Ending  of  element  : %s\n", (char*)name);
#endif
#ifdef TRACE_FUNC_CALLS
    LOG_TRACE_OUT;
#endif
}


static void
my_characters(void *user_data,
              const xmlChar *ch,
              int len)
{
#ifdef TRACE_FUNC_CALLS
    LOG_TRACE_IN;
#endif

    MwsHarvest_SaxUserData* data = (MwsHarvest_SaxUserData*) user_data;

    if (data->state == MWSHARVESTSTATE_IN_MWS_MATH &&   // Valid state
            data->currentToken != NULL) {               // Valid token
        data->currentToken->appendTextContent((char*) ch, len);
    }

    if (data->state == MWSHARVESTSTATE_IN_MWS_COPY) {
        string inputChars((const char*) ch, len);
        xmlChar* encodedChars =
                xmlEncodeSpecialChars(NULL, BAD_CAST inputChars.c_str());
        assert(encodedChars != NULL);
        xmlTextWriterWriteRawLen(data->stringWriter, encodedChars,
                                 strlen((const char*) encodedChars));
        xmlFree(encodedChars);
    }

#ifdef TRACE_FUNC_CALLS
    LOG_TRACE_OUT;
#endif
}


static void
my_warning(void*       user_data,
           const char* msg,
           ...)
{
#ifdef TRACE_FUNC_CALLS
    LOG_TRACE_IN;
#endif

    va_list args;
    MwsHarvest_SaxUserData* data = (MwsHarvest_SaxUserData*) user_data;

    data->warnings++;
    
    va_start(args, msg);
    vfprintf(stderr, msg, args);
    va_end(args);

#ifdef TRACE_FUNC_CALLS
    LOG_TRACE_OUT;
#endif
}


static void
my_error(void*       user_data,
         const char* msg,
         ...)
{
#ifdef TRACE_FUNC_CALLS
    LOG_TRACE_IN;
#endif
    va_list args;
    MwsHarvest_SaxUserData* data = (MwsHarvest_SaxUserData*) user_data;

    data->errorDetected = true;
    if (data->currentTokenRoot)
    {
        delete data->currentTokenRoot;
        data->currentTokenRoot = NULL;
        data->currentToken = NULL;
    }
    va_start(args, msg);
    vfprintf(stderr, msg, args);
    va_end(args);

#ifdef TRACE_FUNC_CALLS
    LOG_TRACE_OUT;
#endif
}


// ATM all errors are caught by error, so this is useless
static void
my_fatalError(void*       user_data,
              const char* msg,
              ...)
{
#ifdef TRACE_FUNC_CALLS
    LOG_TRACE_IN;
#endif
    UNUSED(user_data);

    va_list args;

    va_start(args, msg);
    vfprintf(stderr, msg, args);
    va_end(args);

#ifdef TRACE_FUNC_CALLS
    LOG_TRACE_OUT;
#endif
}


// Implementation

namespace mws
{

pair<int,int>
loadMwsHarvestFromFd(mws::index::IndexManager *indexManager, FILE* fp) {
#ifdef TRACE_FUNC_CALLS
    LOG_TRACE_IN;
#endif

    MwsHarvest_SaxUserData user_data;
    xmlSAXHandler          saxHandler;
    xmlParserCtxtPtr       ctxtPtr;
    int                    ret;

    // Initializing the user_data and return value
    user_data.indexManager = indexManager;
    ret                    = -1;

    // Initializing the SAX Handler
    memset(&saxHandler, 0, sizeof(xmlSAXHandler));

    // Registering Sax callbacks
    saxHandler.startDocument = my_startDocument;
    saxHandler.endDocument   = my_endDocument;
    saxHandler.startElement  = my_startElement;
    saxHandler.endElement    = my_endElement;
    saxHandler.characters    = my_characters;
    saxHandler.warning       = my_warning;
    saxHandler.error         = my_error;
    saxHandler.fatalError    = my_fatalError;

    // Locking libXML -- to allow multi-threaded use
    xmlLockLibrary();
    
    // Creating the IOParser context
    if ((ctxtPtr = xmlCreateIOParserCtxt(&saxHandler,
                                         &user_data,
                                         fileXmlInputReadCallback,
                                         fileXmlInputCloseCallback,
                                         fp,
                                         XML_CHAR_ENCODING_UTF8))
            == NULL) {
        fprintf(stderr, "Error while creating the ParserContext\n");
    }
    // Parsing the document
    else if ((ret = xmlParseDocument(ctxtPtr))
             == -1) {
        fprintf(stderr, "Parsing XML document failed\n");
    }

    // Freeing the parser context
    if (ctxtPtr) {
        xmlFreeParserCtxt(ctxtPtr);
    }

    // Unlocking libXML -- to allow multi-threaded use
    xmlUnlockLibrary();

#ifdef TRACE_FUNC_CALLS
    LOG_TRACE_OUT;
#endif
    return make_pair(ret, user_data.parsedExpr);
}

}
