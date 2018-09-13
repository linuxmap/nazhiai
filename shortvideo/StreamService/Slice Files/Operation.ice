// **********************************************************************
//
// Copyright (c) 2003-2018 ZeroC, Inc. All rights reserved.
//
// **********************************************************************

#pragma once

module stream
{
	class Param
	{
		string file;
	}

    interface Operation
    {
        bool Play(Param param, out string result);
        void Stop(Param param);
    }
}
