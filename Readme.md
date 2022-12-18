# BinaryXML

**BinaryXML** is a binary XML format and single-file library that allows you to bake xml files to a bin file, then load them using a simple library.

## Usage

Here is an example:

```c
// Define BINXML_IMPLEMENTATION in one file like an STB library
#define BINXML_IMPLEMENTATION
#include "binxml.h"
#undef BINXML_IMPLEMENTATION

// You control the memory manegement for documents
BxDocument document;

int main(const int argc, const char *argv[]) {
	int status = BxDocumentLoadFromFile(&document, "mydata.bin");
	
	// Check if there are problems
	if (status != BX_STATUS_SUCCESS) {
		printf("Document load error\n");
		return 1;
	}
	
	// Initialise what we need to read in document
	BxType type;
	void *data;
	
	// Read document until it ends
	while (type != BX_TYPE_EOF) {
		// Read the next element
		status = BxDocumentNext(&document, &type, &data);
		
		// Check for errors
		if (status != BX_STATUS_SUCCESS) {
			printf("BxDocumentNext returned %d before EOF\n", status);
			return status;
		}
		
		// Print out based on type
		switch (type) {
			// BX_TYPE_ATTRIB_END and BX_TYPE_EOF have no data
			case BX_TYPE_ATTRIB_END:
			case BX_TYPE_EOF:
				printf("%3x\n", type);
				break;
			
			// Attributes have a small structure that we should also free
			// Every string is a pointer into the source document memory
			// so they will get freed later.
			case BX_TYPE_ATTRIB:
				printf("%3x \"%s\" \"%s\"\n", type, ((BxAttrib *) data)->key, ((BxAttrib *) data)->value);
				free(data);
				break;
			
			// Everything else just has one string
			default:
				printf("%3x \"%s\"\n", type, (const char *) data);
				break;
		}
	}
	
	// Close the document
	BxDocumentClose(&document);
	
	return 0;
}
```
