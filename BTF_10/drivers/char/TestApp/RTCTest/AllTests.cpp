// RTCTest.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include "CppUTest/CommandLineTestRunner.h"
#include "CppUTest/TestRegistry.h"
#include "CppUTestExt/MemoryReporterPlugin.h"
#include "CppUTestExt/MockSupportPlugin.h"


int _tmain(int ac, _TCHAR** av)
{
	MemoryReporterPlugin plugin;
	MockSupportPlugin mockPlugin;
	TestRegistry::getCurrentRegistry()->installPlugin(&plugin);
	TestRegistry::getCurrentRegistry()->installPlugin(&mockPlugin);

	return CommandLineTestRunner::RunAllTests(ac, av);

	return 0;
}
