// **********************************************************************
//
// Copyright (c) 2003-2018 ZeroC, Inc. All rights reserved.
//
// **********************************************************************

#pragma once

module mux
{
	class Param
	{
		string url;
	}

    interface Operation
    {
        bool Start(Param param, out string result);
				
        void Stop(Param param);

        bool One(string url, string timepoint, out string path);

    }

}
