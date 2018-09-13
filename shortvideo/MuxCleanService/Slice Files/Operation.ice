// **********************************************************************
//
// Copyright (c) 2003-2018 ZeroC, Inc. All rights reserved.
//
// **********************************************************************

#pragma once

module clean
{
	class Param
	{
		int hostid;
		string url;
		string begin;
		string end;
	}

    interface CleanShortVideo
    {
        bool Clean(Param param, out string result);
    }
}
