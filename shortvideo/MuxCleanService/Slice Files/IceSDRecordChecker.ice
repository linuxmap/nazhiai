/**
 * @file IceSDRecordChecker.ice
 * @brief   
 * @author cxl, www.nazhiai.com, <xiaolong.chen@nazhiai.com>
 * @version 3.20180827
 * @date 2018-08-27
 *
 */

#pragma once


#include <Ice/BuiltinSequences.ice>

#include "IceSDBasicTypes.ice"


module SDBusiness
{
    struct SCaptureQueryCond
    {
        long begin;
        long end;           // data set is defined in [begin, end)
        string videoUrl;    // if 'ALL', select all videos
    };
    
	sequence<string> StrSeq;

    interface ISDRecordChecker
    {
        /**
         * @brief  QueryCaptureList Query capture timestamps that match given conditions.
         * @since  V3.20180827
         * @since  V3.20180827
         *
         * @param vQueryCond    Query conditions.
         * @param voTimestamps  Capture timestamps.
         *
         * @return  Success if err is 0. Failure on ohter err codes.
         */
        BasicTypes::SBasicReturn QueryCaptureList(SCaptureQueryCond vQueryCond, out Ice::LongSeq voTimestamps);
        
        
        /**
         * @brief  GetStartedVideos Get video urls which are currently opened.
         * @since  V3.20180827
         * @since  V3.20180827
         *
         * @param voVideoUrls  Video urls.
         *
         * @return  Success if err is 0. Failure on ohter err codes.
         */
        BasicTypes::SBasicReturn GetStartedVideos(out StrSeq voVideoUrls);
    }
}
