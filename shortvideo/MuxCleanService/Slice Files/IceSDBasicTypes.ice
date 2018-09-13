/**
 * @file IceSDBasicTypes.ice
 * @brief   
 * @author cxl, www.nazhiai.com, <xiaolong.chen@nazhiai.com>
 * @version 2.5
 * @date 2018-04-15
 *
 *
 */

#pragma once


module BasicTypes
{
    struct SBasicReturn
    {
        /**
         * @brief  0 if success, others if fail.
         */
        int err;
        /**
         * @brief  msg in utf8.
         */
        string msg;
        /**
         * @brief  是否可以把msg展示给用户。
         */
        bool userVisible;
    };

};
