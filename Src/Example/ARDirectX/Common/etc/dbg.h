#pragma once

namespace common { namespace dbg
{
	void Print( const std::string &str );
	void Print( const char* fmt, ...);

	void Log(const char* fmt, ...);
}}
