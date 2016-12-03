#include "stdafx.h"
#include "CppUnitTest.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace SharkCage {
	TEST_CLASS(SharkCage) {
	public:
        TEST_CLASS_INITIALIZE(initClass) {
            // Runs after the class is genereated to initialize class fields
        }
        TEST_CLASS_CLEANUP(cleanUpClass) {
            // Runs before the class is destroyed to free resources
        }

        TEST_METHOD_INITIALIZE(init) {
            // Runs before each test to set up something
        }
        TEST_METHOD_CLEANUP(cleanUp) {
            // Runs after each test to clean up
        }

		TEST_METHOD(TestMethod1)
		{
			// TODO: Your test code here
            Assert::AreEqual(1.0f, 1.0f, 0.0f, L"Numbers are not equal!");
		}
	};
}
