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
	static bool bInitialized;
    static ELoggerVerbosity Verbosity;
    static FILE* File;

public:
    static void Log(const char* String, ELoggerVerbosity LogVerbosity);
    static void Finish();
};

bool SLogger::bInitialized;
ELoggerVerbosity SLogger::Verbosity;
FILE* SLogger::File;

void SLogger::Log(const char* String, ELoggerVerbosity LogVerbosity)
{   
	if (!bInitialized)
	{
	#ifdef ENGINE_RELEASE
		Verbosity = LoggerVerbosity_Release;
	#elif defined(ENGINE_PROFILE)
		Verbosity = LoggerVerbosity_Debug;
	#else
		Verbosity = LoggerVerbosity_SuperDebug;
	#endif

		File = fopen("Log.txt", "w");
		Assert(File);

		bInitialized = true;
	}

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