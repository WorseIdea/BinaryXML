/**
 * Copyright (C) 2022 Knot126, CC0 licence
 * 
 * BinaryXML
 */

#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum {
	BX_STATUS_SUCCESS = 0,
	BX_STATUS_FAILURE = 1,
	BX_STATUS_BAD_DOCUMENT = 2,
	
	BX_CONTEXT_NONE = 0,
	BX_CONTEXT_OUTERMOST = 1,
	BX_CONTEXT_ATTRIBS = 2,
	
	BX_TYPE_COMMENT = 0,
	BX_TYPE_TAG = 1,
	BX_TYPE_END_TAG = 2,
	BX_TYPE_TEXT = 3,
	BX_TYPE_EOF = 0xff,
	BX_TYPE_ATTRIB_END = 0x100,
	BX_TYPE_ATTRIB = 0x101,
};

typedef int BxType;

typedef struct BxAttrib {
	const char *key;
	const char *value;
} BxAttrib;

typedef struct BxDocument {
	size_t length;
	const char *content;
	
	size_t head;
	int context;
} BxDocument;

#if defined(BINXML_IMPLEMENTATION)
int BxDocumentLoadFromMemory(BxDocument * restrict this, size_t length, const char * restrict content) {
	/**
	 * Load bytes in memory to a BxDocument.
	 * 
	 * @warning You should never free the content you pass in manually, it will be 
	 * freed when the document is freed.
	 * 
	 * @param this Document object
	 * @param length Length of the string of bytes
	 * @param content The content of the document
	 * @return Error code
	 */
	
	memset(this, 0, sizeof *this);
	
	this->length = length;
	this->content = content;
	
	return BX_STATUS_SUCCESS;
}

static size_t BxGetFileLength(FILE *file) {
	/**
	 * Get the length of a file.
	 * 
	 * @param file The file
	 * @return size
	 */
	
	fseek(file, 0, SEEK_END);
	size_t length = ftell(file);
	fseek(file, 0, SEEK_SET);
	
	return length;
}

static const char *BxBufferFromFile(size_t length, FILE *file) {
	/**
	 * Load data from a file
	 * 
	 * @param length
	 * @param file
	 * @return data
	 */
	
	char *buffer = NULL;
	
	// allocate buffer
	buffer = malloc(length);
	
	if (buffer == NULL) {
		return NULL;
	}
	
	// read buffer
	size_t read = fread(buffer, 1, length, file);
	
	if (read != length) {
		free(buffer);
		return NULL;
	}
	
	return buffer;
}

int BxDocumentLoadFromFile(BxDocument * restrict this, const char * restrict path) {
	/**
	 * Load a document from a file.
	 * 
	 * @param this Document object
	 * @param path Path to the file
	 * @return Error code
	 */
	
	memset(this, 0, sizeof *this);
	
	// Open the file
	FILE *f;
	
	f = fopen(path, "rb");
	
	if (f == NULL) {
		goto Error;
	}
	
	// Get the file length
	this->length = BxGetFileLength(f);
	
	if (this->length == 0) {
		goto Error;
	}
	
	// Read to buffer
	this->content = BxBufferFromFile(this->length, f);
	
	if (this->content == NULL) {
		goto Error;
	}
	
	// Close file
	fclose(f);
	
	return BX_STATUS_SUCCESS;
	
Error:
	if (f) {
		fclose(f);
	}
	
	if (this->content) {
		free((void *) this->content);
	}
	
	return BX_STATUS_FAILURE;
}

int BxDocumentClose(BxDocument *this) {
	/**
	 * Close a document.
	 * 
	 * @param this The document to close
	 * @return status
	 */
	
	if (this->content) {
		free((void *) this->content);
	}
	
	return BX_STATUS_SUCCESS;
}

static const char *BxDocumentReadString(BxDocument *this) {
	/**
	 * Read a string in the document.
	 * 
	 * @param this Document object
	 * @param length Pointer to the address storing the document head
	 * @return Pointer to the string
	 */
	
	const char *string = &this->content[this->head];
	
	// Skip past the main string
	while ((this->content[this->head] != '\0') && (this->head < this->length)) {
		this->head++;
	}
	
	// Skip past the NUL terminator
	this->head++;
	
	return string;
}

static int BxDocumentNextAttr(BxDocument *this, BxType *type, void **data) {
	/**
	 * Read the next element in the document in attribute mode.
	 */
	
	char current = this->content[this->head++];
	
	switch (current) {
		// End of attributes
		case 0x00: {
			type[0] = BX_TYPE_ATTRIB_END;
			data[0] = (void *) NULL;
			this->context = BX_CONTEXT_OUTERMOST;
			break;
		}
		
		// There is another attribute
		case 0x01: {
			type[0] = BX_TYPE_ATTRIB;
			
			BxAttrib *attr = malloc(sizeof(BxAttrib));
			
			if (!attr) {
				return BX_STATUS_FAILURE;
			}
			
			attr->key = BxDocumentReadString(this);
			attr->value = BxDocumentReadString(this);
			
			data[0] = (void *) attr;
			break;
		}
		
		default: {
			return BX_STATUS_BAD_DOCUMENT;
		}
	}
	
	return BX_STATUS_SUCCESS;
}

int BxDocumentNext(BxDocument *this, BxType *type, void **data) {
	/**
	 * Read the next element of the document.
	 * 
	 * @param this Document object
	 * @param type The type of element
	 * @param data Data assocaited with this part of the document
	 * @return Error code
	 */
	
	// Do attr context if needed
	if (this->context == BX_CONTEXT_ATTRIBS) {
		return BxDocumentNextAttr(this, type, data);
	}
	
	// Check that our context is right
	if (this->context == BX_CONTEXT_NONE) {
		if (this->head == 0) {
			this->context = BX_CONTEXT_OUTERMOST;
		}
		else {
			return BX_STATUS_FAILURE;
		}
	}
	
	char current = this->content[this->head++];
	
	switch (current) {
		case BX_TYPE_COMMENT: {
			type[0] = BX_TYPE_COMMENT;
			data[0] = (void *) BxDocumentReadString(this);
			break;
		}
		
		case BX_TYPE_TAG: {
			type[0] = BX_TYPE_TAG;
			data[0] = (void *) BxDocumentReadString(this);
			this->context = BX_CONTEXT_ATTRIBS;
			break;
		}
		
		case BX_TYPE_END_TAG: {
			type[0] = BX_TYPE_END_TAG;
			data[0] = (void *) BxDocumentReadString(this);
			break;
		}
		
		case BX_TYPE_TEXT: {
			type[0] = BX_TYPE_TEXT;
			data[0] = (void *) BxDocumentReadString(this);
			break;
		}
		
		case BX_TYPE_EOF: {
			type[0] = BX_TYPE_EOF;
			data[0] = (void *) NULL;
			break;
		}
		
		default: {
			return BX_STATUS_BAD_DOCUMENT;
		}
	}
	
	return BX_STATUS_SUCCESS;
}
#endif
