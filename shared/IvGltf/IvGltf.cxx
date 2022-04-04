#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION

#include "IvGltf.h"
#include "tiny_gltf.h"

#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/actions/SoWriteAction.h>
#include <Inventor/SoOutput.h>
#include <Inventor/SoInput.h>

bool IvGltf::writeFile(std::string filename, SoSeparator* root, bool isBinary) 
{
	SoOutput out;
	if (!out.openFile(filename.c_str())) {
		return false; 
	}
	SoWriteAction wa(&out);
	out.setBinary(isBinary);
	wa.apply(root);
	out.closeFile();
	return true; 
}

SoSeparator* IvGltf::readFile(std::string filename) { 
	SoInput input;
	if (!input.openFile(filename.c_str())) {
		fprintf(stderr, "Cannot open file %s\n", filename.c_str());
		return nullptr;
	}

	// Read the whole file into the database
	SoSeparator* root = SoDB::readAll(&input);
	if (root == nullptr) {
		fprintf(stderr, "Problem reading file\n");	
	}
	input.closeFile();
	return root; 
}
