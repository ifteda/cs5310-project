// Run with ./project "./media/see_you_again.wav"

// Support Code written by Michael D. Shah
// Last Updated: 6/11/21
// Please do not redistribute without asking permission.

// Functionality that we created
#include "SDLGraphicsProgram.hpp"

#include <iostream>

int main(int argc, char** argv) {

	if (argc < 2) {
		std::cout << "Please provide path to .wav file" << std::endl;
		return -1;
	}

	const char* filepath = argv[1];

	// Create an instance of an object for a SDLGraphicsProgram
	SDLGraphicsProgram mySDLGraphicsProgram(1280, 720, filepath);
	// Run our program forever
	mySDLGraphicsProgram.Loop();
	// When our program ends, it will exit scope, the
	// destructor will then be called and clean up the program.
	return 0;
}
