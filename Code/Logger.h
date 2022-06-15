#pragma once

enum ELoggerVerbosity
{
    LoggerVerbosity_SuperDebug,
    LoggerVerbosity_Debug,
    LoggerVerbosity_Release,
};

struct SLogger
{
private:
    static ELoggerVerbosity Verbosity;
    static FILE* File;

public:
    static void Initialize(ELoggerVerbosity VerbosityInit);
    static void Log(const char* String, ELoggerVerbosity LogVerbosity);
    static void Finish();
};

ELoggerVerbosity SLogger::Verbosity;
FILE* SLogger::File;

void SLogger::Initialize(ELoggerVerbosity VerbosityInit)
{
    Verbosity = VerbosityInit;

    File = fopen("Log.txt", "w");
    Assert(File);
}

void SLogger::Log(const char* String, ELoggerVerbosity LogVerbosity)
{   
	if (LogVerbosity >= Verbosity)
	{
		if (File)
		{
			fwrite(String, StringLength(String), 1, File);
			fflush(File);
		}

#ifndef ENGINE_RELEASE
		PlatformOutputDebugString(String);
#endif
	}
}

void SLogger::Finish()
{
	if (File)
	{
    	fclose(File);
		File = 0;
	}
}