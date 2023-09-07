---- #########################################################################
---- #                                                                       #
---- # Copyright (C) OpenTX                                                  #
-----#                                                                       #
---- # License GPLv2: http://www.gnu.org/licenses/gpl-2.0.html               #
---- #                                                                       #
---- # This program is free software; you can redistribute it and/or modify  #
---- # it under the terms of the GNU General Public License version 2 as     #
---- # published by the Free Software Foundation.                            #
---- #                                                                       #
---- # This program is distributed in the hope that it will be useful        #
---- # but WITHOUT ANY WARRANTY; without even the implied warranty of        #
---- # MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         #
---- # GNU General Public License for more details.                          #
---- #                                                                       #
---- #########################################################################

------------------------------------------------------------------------------
-- This script simulates the Forward programming menus for AR631 and FC6250HX 
-- receivers.
-- The intend is to make easier GUI development in Companion since it cannot
-- talk to the receivers
--
-- Author: Francisco Arzu 
------------------------------------------------------------------------------


local Log, menuLib, modelLib, DEBUG_ON = ... -- Get DebugON from parameters
local SIM_LIB_VERSION = "0.55"
local MSG_FILE = "/SCRIPTS/TOOLS/DSMLIB/msg_fwdp_en.txt"

local PHASE = menuLib.PHASE
local LINE_TYPE = menuLib.LINE_TYPE

local SimLib = {}

local lastGoodMenu=0
local RX_loadMenu = nil
local RX_Initialized =  true

local IS_EDGETX = false


local function AR631_loadMenu(menuId)
    menuLib.clearMenuLines()
    local ctx = menuLib.DSM_Context

    if (menuId==0x1000) then
        --M[Id=0x1000 P=0x0 N=0x0 B=0x0 Text="Main Menu"]
        --L[#0 T=M VId=0x1010 Text="Gyro settings" MId=0x1000 ]
        --L[#1 T=M VId=0x105E Text="Other settings" MId=0x1000 ]

        ctx.Menu = { MenuId = 0x1000, TextId = 0x004B, PrevId = 0, NextId = 0, BackId = 0 }
        ctx.MenuLines[0] = { Type = LINE_TYPE.MENU, TextId = 0x00F9, ValId = 0x1010 }
        ctx.MenuLines[1] = { Type = LINE_TYPE.MENU, TextId = 0x0227, ValId = 0x105E }
        ctx.SelLine = 0
        lastGoodMenu = menuId
    elseif (menuId==0x1010) then
      -- M[Id=0x1010 P=0x0 N=0x0 B=0x1000 Text="Gyro settings"]

        -- NEW
        -- L[#5 T=M VId=0x104F val=nil [0->0,3] Text="First Time Setup" MId=0x1010 ]      -- NEW ONLY
        -- L[#6 T=M VId=0x1055 Text="First Time SAFE Setup"[0x20D]   MId=0x1010 ]

        -- Initialize AR637T
        -- L[#0 T=M VId=0x1011 Text="AS3X Settings"[0x1DD]  MId=0x1010 ]  
        -- L[#1 T=M VId=0x1019 Text="SAFE Settings"[0x1E2]  MId=0x1010 ]
        -- L[#2 T=M VId=0x1021 Text="F-Mode Setup"[0x87]    MId=0x1010 ]
        -- L[#3 T=M VId=0x1022 Text="System Setup"[0x86]    MId=0x1010 ]
        -- Only on BNF locked receivers 
        -- L[#4 T=M VId=0x105C Text="SAFE Select "[0x1F9]   MId=0x1010 ]


        ctx.Menu = { MenuId = 0x1010, TextId = 0x00F9, PrevId = 0, NextId = 0, BackId = 0x1000 }
        if not RX_Initialized then 
            ctx.MenuLines[5] = { Type = LINE_TYPE.MENU, TextId = 0x00A5, ValId = 0x104F}
            ctx.MenuLines[6] = { Type = LINE_TYPE.MENU, TextId = 0x020D, ValId = 0x1055}
            ctx.SelLine = 5
        else
            ctx.MenuLines[0] = { Type = LINE_TYPE.MENU, TextId = 0x01DD, ValId = 0x1011 }
            ctx.MenuLines[1] = { Type = LINE_TYPE.MENU, TextId = 0x01E2, ValId = 0x1019 }
            ctx.MenuLines[2] = { Type = LINE_TYPE.MENU, TextId = 0x0087, ValId = 0x1021 }
            ctx.MenuLines[3] = { Type = LINE_TYPE.MENU, TextId = 0x0086, ValId = 0x1022 }
            ctx.MenuLines[4] = { Type = LINE_TYPE.MENU, TextId = 0x01F9, ValId = 0x105C }
            ctx.SelLine = 0
        end
        lastGoodMenu = menuId
    elseif (menuId==0x1011) then
        -- M[Id=0x1011 P=0x0 N=0x0 B=0x1010 Text="AS3X Settings"[0x1DD]]
        -- L[#0 T=M VId=0x1012  Text="AS3X Gains"[0x1DE] MId=0x1011 ]
        -- L[#1 T=M VId=0x1013  Text="Priority"[0x46] MId=0x1011 ]

        -- L[#2 T=M VId=0x1015  Text="Heading"[0x82] MId=0x1011 ]
        -- L[#4 T=LM VId=0x1004 val=50 [0->244,50,TS=0] Text="Gain Sensitivity"[0x8A] MId=0x1011]
        -- L[#5 T=M VId=0x1016  Text="Fixed/Adjustable Gains"[0x263] MId=0x1011 ]   
        -- L[#6 T=M VId=0x1017  Text="Capture Gyro Gains"[0xAA] MId=0x1011 ]     

        ctx.Menu = { MenuId = 0x1011, TextId = 0x1DD, PrevId = 0, NextId = 0, BackId = 0x1010 }
        ctx.MenuLines[0] = { Type = LINE_TYPE.MENU, TextId = 0x1DE, ValId = 0x1012}
        ctx.MenuLines[1] = { Type = LINE_TYPE.MENU, TextId = 0x46, ValId = 0x1013}
        ctx.MenuLines[2] = { Type = LINE_TYPE.MENU, TextId = 0x82, ValId = 0x1015}
        ctx.MenuLines[4] = { Type = LINE_TYPE.LIST_MENU, TextId = 0x8A, ValId = 0x1004, Min=0, Max=244, Def=50, Val=50 }
        ctx.MenuLines[5] = { Type = LINE_TYPE.MENU, TextId = 0x263, ValId = 0x1016}
        ctx.MenuLines[6] = { Type = LINE_TYPE.MENU, TextId = 0xAA, ValId = 0x1017 }
        ctx.SelLine = 0
        lastGoodMenu = menuId
    elseif (menuId==0x1012) then
        -- M[Id=0x1012 P=0x0 N=0x0 B=0x1011 Text="AS3X Gains"[0x1DE]]
        --L[#0 T=V_nc VId=0x1000 Text="Flight Mode 1"[0x8001] val=1 [0->10,0] MId=0x1012 ]
        --L[#2 T=M    VId=0x1012 Text="Rate Gains"[0x1E0] MId=0x1012 ]
        --L[#3 T=V_nc VId=0x1004 Text="Roll"[0x40] val=14 [0->100,40] MId=0x1012 ]
        --L[#4 T=V_nc VId=0x1005 Text="Pitch"[0x41] val=29 [0->100,50] MId=0x1012 ]
        --L[#5 T=V_nc VId=0x1006 Text="Yaw"[0x42]  val=48 [0->100,60] MId=0x1012 ]
    
        ctx.Menu = { MenuId = 0x1012, TextId = 0x1DE, PrevId = 0, NextId = 0, BackId = 0x1011 }
        ctx.MenuLines[0] = { Type = LINE_TYPE.VALUE_NUM_I8_NC, TextId = 0x8001, ValId = 0x1000, Min=0, Max=10, Def=0, Val=1 }
        ctx.MenuLines[2] = { Type = LINE_TYPE.MENU, TextId = 0x1E0, ValId = 0x1012 }
        ctx.MenuLines[3] = { Type = LINE_TYPE.VALUE_NUM_I8_NC, TextId = 0x40, ValId = 0x1004, Min=0, Max=100, Def=40, Val=40 }
        ctx.MenuLines[4] = { Type = LINE_TYPE.VALUE_NUM_I8_NC, TextId = 0x41, ValId = 0x1005, Min=0, Max=100, Def=50, Val=50 }
        ctx.MenuLines[5] = { Type = LINE_TYPE.VALUE_NUM_I8_NC, TextId = 0x42, ValId = 0x1006, Min=0, Max=100, Def=60, Val=60 }

        ctx.SelLine = 3
        lastGoodMenu = menuId
    elseif (menuId==0x1013) then
        --M[Id=0x1013 P=0x0 N=0x0 B=0x1011 Text="Priority"[0x46]]
        --L[#0 T=V_nc VId=0x1000 Text="Flight Mode 1"[0x8001] val=1 [0->10,0] MId=0x1012 ]
        --L[#1 T=M VId=0x1013 Text="Stick Priority"[0xFE] MId=0x1013 ]
        --L[#3 T=V_nc VId=0x1004  Text="Roll"[0x40] val=14 [0->160,160] MId=0x1012 ]
        --L[#4 T=V_nc VId=0x1005  Text="Pitch"[0x41] val=29 [0->160,160] MId=0x1012 ]
        --L[#5 T=V_nc VId=0x1006  Text="Yaw"[0x42] val=48 [0->160,160] MId=0x1012 ]
    
        ctx.Menu = { MenuId = 0x1013, TextId = 0x46, PrevId = 0, NextId = 0, BackId = 0x1011 }
        ctx.MenuLines[0] = { Type = LINE_TYPE.VALUE_NUM_I8_NC, TextId = 0x8001, ValId = 0x1000, Min=0, Max=10, Def=0, Val=1 }
        ctx.MenuLines[2] = { Type = LINE_TYPE.MENU, TextId = 0xFE, ValId = 0x1013 }
        ctx.MenuLines[3] = { Type = LINE_TYPE.VALUE_NUM_I8_NC, TextId = 0x40, ValId = 0x1004, Min=0, Max=160, Def=100, Val=160 }
        ctx.MenuLines[4] = { Type = LINE_TYPE.VALUE_NUM_I8_NC, TextId = 0x41, ValId = 0x1005, Min=0, Max=160, Def=100, Val=160 }
        ctx.MenuLines[5] = { Type = LINE_TYPE.VALUE_NUM_I8_NC, TextId = 0x42, ValId = 0x1006, Min=0, Max=160, Def=100, Val=160 }

        ctx.SelLine = 3
        lastGoodMenu = menuId
    elseif (menuId==0x1015) then
        -- M[Id=0x1015 P=0x0 N=0x0 B=0x1011 Text="Heading Gain"[0x266]]
        -- L[#0T=V_nc VId=0x1000 Text="Flight Mode 1"[0x8001] val=1 [0->10,0] MId=0x1015 ]
        -- L[#1 T=M VId=0x1015 Text="Heading Gain"[0x266] MId=0x1015 ]
        -- L[#2 T=V_nc VId=0x1004 Text="Roll"[0x40] val=0 [0->100,0] MId=0x1015  ]
        -- L[#3 T=V_nc VId=0x1005 Text="Pitch"[0x41] val=0 [0->100,0] MId=0x1015 ]
        -- L[#5 T=M VId=0x1015 Text="Use CAUTION for Yaw gain!"[0x26A] MId=0x1015 ]
        -- L[#6T=V_nc VId=0x1006 Text="Yaw"[0x42] val=0 [0->100,0] MId=0x1015 ]

        ctx.Menu = { MenuId = 0x1015, TextId = 0x266, PrevId = 0, NextId = 0, BackId = 0x1011 }
        ctx.MenuLines[0] = { Type = LINE_TYPE.VALUE_NUM_I8_NC, TextId = 0x8001, ValId = 0x1000, Min=0, Max=10, Def=0, Val=1 }
        ctx.MenuLines[1] = { Type = LINE_TYPE.MENU, TextId = 0x1F9, ValId = 0x1015 }
        ctx.MenuLines[2] = { Type = LINE_TYPE.VALUE_NUM_I8_NC, TextId = 0x40, ValId = 0x1004, Min=0, Max=100, Def=0, Val=0 }
        ctx.MenuLines[3] = { Type = LINE_TYPE.VALUE_NUM_I8_NC, TextId = 0x41, ValId = 0x1005, Min=0, Max=100, Def=0, Val=0 }
        ctx.MenuLines[5] = { Type = LINE_TYPE.MENU, TextId = 0x26A, ValId = 0x1015 }
        ctx.MenuLines[6] = { Type = LINE_TYPE.VALUE_NUM_I8_NC, TextId = 0x42, ValId = 0x1006, Min=0, Max=100, Def=0, Val=0 }
        ctx.SelLine = 0
        lastGoodMenu = menuId
    elseif (menuId==0x1016) then
        -- M[Id=0x1016 P=0x0 N=0x0 B=0x1011 Text="Fixed/Adjustable Gains"[0x263]]
        -- L[#0T=V_nc VId=0x1000 Text="Flight Mode 1"[0x8001] val=1 [0->10,0]  MId=0x1016 ]
        -- L[#1 T=M VId=0x1016 val=nil [0->0,2] Text="Fixed/Adjustable Gains"[0x263] MId=0x1016 ]
        -- L[#2 T=LM_nc VId=0x1002 Text="Roll"[0x40] MId=0x1016 val=1 NL=(0->1,24,S=242) [242->243,243] ]
        -- L[#3 T=LM_nc VId=0x1003 Text="Pitch"[0x41] MId=0x1016 val=1 NL=(0->1,1,S=242) [242->243,243] ]
        -- L[#4 T=LM_nc VId=0x1004  Text="Yaw"[0x42] MId=0x1016 val=1 NL=(0->1,1,S=242) [242->243,243] ]

        ctx.Menu = { MenuId = 0x1016, TextId = 0x263, PrevId = 0, NextId = 0, BackId = 0x1011 }
        ctx.MenuLines[0] = { Type = LINE_TYPE.VALUE_NUM_I8_NC, TextId = 0x8001, ValId = 0x1000, Min=0, Max=10, Def=0, Val=1 }
        ctx.MenuLines[1] = { Type = LINE_TYPE.MENU, TextId = 0x1F9, ValId = 0x1016 }
        ctx.MenuLines[2] = { Type = LINE_TYPE.LIST_MENU_NC, TextId = 0x40, ValId = 0x1002, Min=242, Max=243, Def=243, Val=1 }
        ctx.MenuLines[3] = { Type = LINE_TYPE.LIST_MENU_NC, TextId = 0x41, ValId = 0x1003, Min=242, Max=243, Def=243, Val=1 }
        ctx.MenuLines[4] = { Type = LINE_TYPE.LIST_MENU_NC, TextId = 0x42, ValId = 0x1004, Min=242, Max=243, Def=243, Val=1 }
        ctx.SelLine = 0
        lastGoodMenu = menuId
    elseif (menuId==0x1017) then
        --M[Id=0x1017 P=0x0 N=0x0 B=0x1011 Text="Capture Gyro Gains"[0xAA]]
        --L[#0 T=M VId=0x1017 Text="Gains will be captured on"[0x24C]   MId=0x1017 ]
        --L[#1 T=V_nc VId=0x1000 Text="                  Flight Mode 1"[0x8001] Val=1 [0->10,0] MId=0x1017 ]
        --L[#2 T=M VId=0x1017 Text="Captured gains will be"[0x24D]   MId=0x1017 ]
        --L[#3 T=V_i8 VId=0x1004 Text="Roll"[0x40] Val=40 [0->0,0] MId=0x1017 ]
        --L[#4 T=V_i8 VId=0x1005 Text="Pitch"[0x41] Val=50 [0->0,0] MId=0x1017 ]
        --L[#5 T=V_i8 VId=0x1006 Text="Yaw"[0x42] Val=60 [0->0,0] MId=0x1017 ]
        --L[#6 T=M VId=0x1018 Text="Capture Gyro Gains"[0xAA]   MId=0x1017 ]

        ctx.Menu = { MenuId = 0x1017, TextId = 0xAA, PrevId = 0, NextId = 0, BackId = 0x1011 }
        ctx.MenuLines[0] = { Type = LINE_TYPE.MENU, TextId = 0x24C, ValId = 0x1017 }
        ctx.MenuLines[1] = { Type = LINE_TYPE.VALUE_NUM_I8_NC, TextId = 0x8001, ValId = 0x1000, Min=0, Max=10, Def=0, Val=1 }
        ctx.MenuLines[2] = { Type = LINE_TYPE.MENU, TextId = 0x24D, ValId = 0x1017 }
        ctx.MenuLines[3] = { Type = LINE_TYPE.VALUE_NUM_I8, TextId = 0x40, ValId = 0x1004, Min=0, Max=0, Def=0, Val=40 }
        ctx.MenuLines[4] = { Type = LINE_TYPE.VALUE_NUM_I8, TextId = 0x41, ValId = 0x1005, Min=0, Max=0, Def=0, Val=50 }
        ctx.MenuLines[5] = { Type = LINE_TYPE.VALUE_NUM_I8, TextId = 0x42, ValId = 0x1006, Min=0, Max=0, Def=0, Val=60 }
        ctx.MenuLines[6] = { Type = LINE_TYPE.MENU, TextId = 0xAA, ValId = 0x1018 }

        ctx.SelLine = 6
        lastGoodMenu = menuId
    elseif (menuId==0x1018) then
        --M[Id=0x1018 P=0x0 N=0x0 B=0x1011 Text="Capture Gyro Gains"[0xAA]]
        --L[#0 T=M VId=0x1018 Text="Gains on"[0x24E]   MId=0x1018 ]
        --L[#1 T=V_nc VId=0x1018 Text="                  Flight Mode 1"[0x8001] Val=1 [0->10,0] MId=0x1018 ]
        --L[#2 T=M VId=0x1018 Text="were captured and changed"[0x24F]   MId=0x1018 
        --L[#3 T=M VId=0x1018 Text="from Adjustable to Fixed"[0x250]   MId=0x1018 ]
        --L[#4 T=V_i8 VId=0x1004 Text="Roll"[0x40] Val=40 [0->0,0] MId=0x1018 ]
        --L[#5 T=V_i8 VId=0x1005 Text="Pitch"[0x41] Val=50 [0->0,0] MId=0x1018 ]
        --L[#6 T=V_i8 VId=0x1006 Text="Yaw"[0x42] Val=60 [0->0,0] MId=0x1018 ]

        ctx.Menu = { MenuId = 0x1018, TextId = 0xAA, PrevId = 0, NextId = 0, BackId = 0x1011 }
        ctx.MenuLines[0] = { Type = LINE_TYPE.MENU, TextId = 0x24E, ValId = 0x1018 }
        ctx.MenuLines[1] = { Type = LINE_TYPE.VALUE_NUM_I8_NC, TextId = 0x8001, ValId = 0x1000, Min=0, Max=10, Def=0, Val=1 }
        ctx.MenuLines[2] = { Type = LINE_TYPE.MENU, TextId = 0x24F, ValId = 0x1018 }
        ctx.MenuLines[3] = { Type = LINE_TYPE.MENU, TextId = 0x250, ValId = 0x1018 }
        ctx.MenuLines[4] = { Type = LINE_TYPE.VALUE_NUM_I8, TextId = 0x40, ValId = 0x1004, Min=0, Max=0, Def=0, Val=40 }
        ctx.MenuLines[5] = { Type = LINE_TYPE.VALUE_NUM_I8, TextId = 0x41, ValId = 0x1005, Min=0, Max=0, Def=0, Val=50 }
        ctx.MenuLines[6] = { Type = LINE_TYPE.VALUE_NUM_I8, TextId = 0x42, ValId = 0x1006, Min=0, Max=0, Def=0, Val=60 }

        ctx.SelLine = -1
        lastGoodMenu = menuId
    elseif (menuId==0x1019) then  
        --M[Id=0x1019 P=0x0 N=0x0 B=0x1010 Text="SAFE Settings"[0x1E2]]
        --L[#0 T=M VId=0x101A Text="SAFE Gains"[0x1E3] MId=0x1019 ]
        --L[#1 T=M VId=0x101B Text="Angle Limits"[0x226] MId=0x1019 ]
        --L[#5 T=M VId=0x101E Text="Fixed/Adjustable Gains"[0x263] MId=0x1019 ]
        --L[#6 T=M VId=0x101F Text="Capture Gyro Gains"[0xAA] MId=0x1019 ]

        ctx.Menu = { MenuId = 0x1019, TextId = 0x1E2, PrevId = 0, NextId = 0, BackId = 0x1010  }
        ctx.MenuLines[0] = { Type = LINE_TYPE.MENU, TextId = 0x1E3, ValId = 0x101A }
        ctx.MenuLines[1] = { Type = LINE_TYPE.MENU, TextId = 0x226, ValId = 0x101B }
        ctx.MenuLines[5] = { Type = LINE_TYPE.MENU, TextId = 0x263, ValId = 0x101E }
        ctx.MenuLines[6] = { Type = LINE_TYPE.MENU, TextId = 0xAA, ValId = 0x101F }
        ctx.SelLine = 0
        lastGoodMenu = menuId
    elseif (menuId==0x101A) then
        --M[Id=0x101A P=0x0 N=0x0 B=0x1019 Text="SAFE Gains"[0x1E3]]
        --L[#0 T=V_nc VId=0x1000 Text="                  Flight Mode 1"[0x8001] Val=nil [0->10,0] MId=0x101A ]
        --L[#1 T=M VId=0x101A Text="Gain"[0x43]   MId=0x101A ]
        --L[#2 T=V_nc VId=0x1002 Text="Roll"[0x40] Val=35 [5->100,35] MId=0x101A ]
        --L[#3 T=V_nc VId=0x1003 Text="Pitch"[0x41] Val=35 [5->100,35] MId=0x101A ]

        ctx.Menu = { MenuId = 0x101A, TextId = 0x1E3, PrevId = 0, NextId = 0, BackId = 0x1019 }
        ctx.MenuLines[0] = { Type = LINE_TYPE.VALUE_NUM_I8_NC, TextId = 0x8001, ValId = 0x1000, Min=0, Max=10, Def=0, Val=1 }
        ctx.MenuLines[1] = { Type = LINE_TYPE.MENU, TextId = 0x43, ValId = 0x101A }
        ctx.MenuLines[2] = { Type = LINE_TYPE.VALUE_NUM_I8_NC, TextId = 0x40, ValId = 0x1002, Min=5, Max=100, Def=35, Val=35 }
        ctx.MenuLines[3] = { Type = LINE_TYPE.VALUE_NUM_I8_NC, TextId = 0x41, ValId = 0x1003, Min=5, Max=100, Def=60, Val=35 }
    
        ctx.SelLine = -1
        lastGoodMenu = menuId
    elseif (menuId==0x101B) then
        --M[Id=0x101B P=0x0 N=0x0 B=0x1019 Text="Angle Limits"[0x226]]
        --L[#0T=V_nc VId=0x1000 Text="                  Flight Mode 1"[0x8001] Val=nil [0->10,0] MId=0x101B ]
        --L[#1 T=M VId=0x101B Text="Angle Limits"[0x226]   MId=0x101B ]
        --L[#2 T=V_nc VId=0x1002 Text="Roll Right"[0x1E9] Val=60 [10->90,60] MId=0x101B ]
        --L[#3 T=V_nc VId=0x1003 Text="Roll Left"[0x1EA] Val=60 [10->90,60] MId=0x101B ]
        --L[#4 T=V_nc VId=0x1004 Text="Pitch Down"[0x1EB] Val=40 [10->75,40] MId=0x101B ]
        --L[#5 T=V_nc VId=0x1005 Text="Pitch Up"[0x1EC] Val=50 [10->75,50] MId=0x101B ]

        ctx.Menu = { MenuId = 0x101B, TextId = 0x226, PrevId = 0, NextId = 0, BackId = 0x1019 }
        ctx.MenuLines[0] = { Type = LINE_TYPE.VALUE_NUM_I8_NC, TextId = 0x8001, ValId = 0x1000, Min=0, Max=10, Def=0, Val=1 }
        ctx.MenuLines[1] = { Type = LINE_TYPE.MENU, TextId = 0x226, ValId = 0x101B }
        ctx.MenuLines[2] = { Type = LINE_TYPE.VALUE_NUM_I8_NC, TextId = 0x1E9, ValId = 0x1002, Min=10, Max=90, Def=60, Val=60 }
        ctx.MenuLines[3] = { Type = LINE_TYPE.VALUE_NUM_I8_NC, TextId = 0x1EA, ValId = 0x1003, Min=10, Max=90, Def=60, Val=60 }
        ctx.MenuLines[4] = { Type = LINE_TYPE.VALUE_NUM_I8_NC, TextId = 0x1EB, ValId = 0x1004, Min=10, Max=90, Def=40, Val=40 }
        ctx.MenuLines[5] = { Type = LINE_TYPE.VALUE_NUM_I8_NC, TextId = 0x1EC, ValId = 0x1005, Min=10, Max=90, Def=50, Val=50 }
    
        ctx.SelLine = -1
        lastGoodMenu = menuId

    elseif (menuId==0x101E) then
        --M[Id=0x101E P=0x0 N=0x0 B=0x1019 Text="Fixed/Adjustable Gains"[0x263]]
        --L[#0 T=V_nc VId=0x1000 Text="                  Flight Mode 1"[0x8001] Val=nil [0->10,0] MId=0x101E ]
        --L[#1 T=M VId=0x101E Text="Fixed/Adjustable Gains"[0x263]   MId=0x101E ]
        --L[#2 T=LM_nc VId=0x1002 Text="Roll"[0x40] Val=0 N=(0->1,1,S=242) [242->243,243] MId=0x101E ]
        --L[#3 T=LM_nc VId=0x1003 Text="Pitch"[0x41] Val=0 N=(0->1,1,S=242) [242->243,243] MId=0x101E ]

        ctx.Menu = { MenuId = 0x101E, TextId = 0x263, PrevId = 0, NextId = 0, BackId = 0x1019 }
        ctx.MenuLines[0] = { Type = LINE_TYPE.VALUE_NUM_I8_NC, TextId = 0x8001, ValId = 0x1000, Min=0, Max=10, Def=0, Val=1 }
        ctx.MenuLines[1] = { Type = LINE_TYPE.MENU, TextId = 0x263, ValId = 0x101E }
        ctx.MenuLines[2] = { Type = LINE_TYPE.LIST_MENU_NC, TextId = 0x40, ValId = 0x1002, Min=242, Max=243, Def=243, Val=0 }
        ctx.MenuLines[3] = { Type = LINE_TYPE.LIST_MENU_NC, TextId = 0x41, ValId = 0x1003, Min=242, Max=243, Def=243, Val=0 }

        ctx.SelLine = -1
        lastGoodMenu = menuId
    elseif (menuId==0x101F) then
        --M[Id=0x101F P=0x0 N=0x0 B=0x1019  Text="Capture Gyro Gains"[0xAA]]
        --L[#0 T=M VId=0x101F Text="Gains will be captured on"[0x24C]   MId=0x101F ]
        --L[#1 T=V_nc VId=0x1000 Text="                  Flight Mode 1"[0x8001] Val=1 [0->10,0] MId=0x101F ]
        --L[#2 T=M VId=0x101F Text="Captured gains will be"[0x24D]   MId=0x101F ]
        --L[#3 T=V_i8 VId=0x1004 Text="Roll"[0x40] Val=40 [0->0,0] MId=0x101F ]
        --L[#4 T=V_i8 VId=0x1005 Text="Pitch"[0x41] Val=50 [0->0,0] MId=0x101F ]
        --L[#6 T=M VId=0x1020 Text="Capture Gyro Gains"[0xAA]   MId=0x101F ]

        ctx.Menu = { MenuId = 0x101F, TextId = 0xAA, PrevId = 0, NextId = 0, BackId = 0x1019  }
        ctx.MenuLines[0] = { Type = LINE_TYPE.MENU, TextId = 0x24C, ValId = 0x101F }
        ctx.MenuLines[1] = { Type = LINE_TYPE.VALUE_NUM_I8_NC, TextId = 0x8001, ValId = 0x1000, Min=0, Max=10, Def=0, Val=1 }
        ctx.MenuLines[2] = { Type = LINE_TYPE.MENU, TextId = 0x24D, ValId = 0x101F }
        ctx.MenuLines[3] = { Type = LINE_TYPE.VALUE_NUM_I8, TextId = 0x40, ValId = 0x1004, Min=0, Max=0, Def=0, Val=35 }
        ctx.MenuLines[4] = { Type = LINE_TYPE.VALUE_NUM_I8, TextId = 0x41, ValId = 0x1005, Min=0, Max=0, Def=0, Val=35 }
        ctx.MenuLines[6] = { Type = LINE_TYPE.MENU, TextId = 0xAA, ValId = 0x1020 }

        ctx.SelLine = 6
        lastGoodMenu = menuId
    elseif (menuId==0x1020) then
        --M[Id=0x1020 P=0x0 N=0x0 B=0x101F Text="Capture Gyro Gains"[0xAA]]
        --L[#0 T=M VId=0x1020 Text="Gains on"[0x24E]   MId=0x1020 ]
        --L[#1 T=V_nc VId=0x1018 Text="                  Flight Mode 1"[0x8001] Val=1 [0->10,0] MId=0x1020 ]
        --L[#2 T=M VId=0x1018 Text="were captured and changed"[0x24F]   MId=0x1020 
        --L[#3 T=M VId=0x1018 Text="from Adjustable to Fixed"[0x250]   MId=0x1020 ]
        --L[#4 T=V_i8 VId=0x1004 Text="Roll"[0x40] Val=40 [0->0,0] MId=0x1020 ]
        --L[#5 T=V_i8 VId=0x1005 Text="Pitch"[0x41] Val=50 [0->0,0] MId=0x1020 ]

        ctx.Menu = { MenuId = 0x1020, TextId = 0xAA, PrevId = 0, NextId = 0, BackId = 0x1019 }
        ctx.MenuLines[0] = { Type = LINE_TYPE.MENU, TextId = 0x24E, ValId = 0x1020 }
        ctx.MenuLines[1] = { Type = LINE_TYPE.VALUE_NUM_I8_NC, TextId = 0x8001, ValId = 0x1000, Min=0, Max=10, Def=0, Val=1 }
        ctx.MenuLines[2] = { Type = LINE_TYPE.MENU, TextId = 0x24F, ValId = 0x1020 }
        ctx.MenuLines[3] = { Type = LINE_TYPE.MENU, TextId = 0x250, ValId = 0x1020 }
        ctx.MenuLines[4] = { Type = LINE_TYPE.VALUE_NUM_I8, TextId = 0x40, ValId = 0x1004, Min=0, Max=0, Def=0, Val=35 }
        ctx.MenuLines[5] = { Type = LINE_TYPE.VALUE_NUM_I8, TextId = 0x41, ValId = 0x1005, Min=0, Max=0, Def=0, Val=35 }

        ctx.SelLine = -1
        lastGoodMenu = menuId
    elseif (menuId==0x1021) then
        --M[Id=0x1021 P=0x0 N=0x0 B=0x1010 Text="F-Mode Setup"[0x87]]
        --L[#0 T=V_nc VId=0x1000 Text="Flight Mode 1"[0x8001] val=1 [0->10,0] MId=0x1021 ]
        --L[#1 T=M VId=0x7CA6 Text="FM Channel"[0x78] MId=0x1021 ]
        --L[#2 T=LM VId=0x1002 Text="AS3X"[0x1DC] val=1 (0->1,3,S=3) [3->4|3] MId=0x1021]
        
        -- Why Jump from Value 3 to 176??  where do we know valid values????
        --L[#3 T=LM VId=0x1003 Text="Safe Mode"[0x1F8] val=3|"Inh" NL=(0->244,0,S=0) [0->244,3]  MId=0x1021 ]
        --L[#3 T=LM VId=0x1003 Text="Safe Mode"[0x1F8] val=176|"Self-Level/Angle Dem" NL=(0->244,0,S=0) [0->244,3]  MId=0x1021 ]
        
        --L[#4 T=LM VId=0x1004 Text="Panic"[0x8B] val=0 NL=(0->1,3,S=3) [3->4,3] MId=0x1021 ]
        --L[#5 T=LM VId=0x1005 Text="High Thr to Pitch"[0x1F0]  val=0 NL=(0->1,3,S=3) [3->4,3] MId=0x1021 ]
        --L[#6 T=LM VId=0x1006 Text="Low Thr to Pitch"[0x1EF] val=0 NL=(0->1,3,S=3) [3->4,3] MId=0x1021 ]

         ctx.Menu = { MenuId = 0x1021, TextId = 0x87, PrevId = 0, NextId = 0, BackId = 0x1010 }
         ctx.MenuLines[0] = { Type = LINE_TYPE.VALUE_NUM_I8_NC, TextId = 0x8001, ValId = 0x1000, Min=0, Max=10, Def=0, Val=1 }
         ctx.MenuLines[1] = { Type = LINE_TYPE.MENU, TextId = 0x78, ValId = 0x7CA6 }
         ctx.MenuLines[2] = { Type = LINE_TYPE.LIST_MENU, TextId = 0x1DC, ValId = 0x1002, Min=3, Max=4, Def=3, Val=1 }
         ctx.MenuLines[3] = { Type = LINE_TYPE.LIST_MENU, TextId = 0x1F8, ValId = 0x1003, Min=0, Max=244, Def=3, Val=176 }
         ctx.MenuLines[4] = { Type = LINE_TYPE.LIST_MENU, TextId = 0x8B, ValId = 0x1004, Min=3, Max=4, Def=3, Val=0 }
         ctx.MenuLines[5] = { Type = LINE_TYPE.LIST_MENU, TextId = 0x1F0, ValId = 0x1005, Min=3, Max=4, Def=3, Val=0 }
         ctx.MenuLines[6] = { Type = LINE_TYPE.LIST_MENU, TextId = 0x1EF, ValId = 0x1006, Min=3, Max=4, Def=3, Val=0 }
         ctx.SelLine = 1
         lastGoodMenu = menuId
    elseif (menuId==0x1022) then  
        --M[Id=0x1022 P=0x0 N=0x0 B=0x1010 Text="System Setup"[0x86]]
        --L[#0 T=M VId=0x1023 Text="Relearn Servo Settings"[0x190] MId=0x1022 ]
        --L[#1 T=M VId=0x1025 Text="Orientation"[0x80] MId=0x1022 ]
        --L[#2 T=M VId=0x1029 Text="Gain Channel Select"[0xAD] MId=0x1022 ]
        --L[#3 T=M VId=0x102A Text="SAFE/Panic Mode Setup"[0xCA] MId=0x1022 ]
        --L[#4 T=M VId=0x1032 Text="Utilities"[0x240] MId=0x1022 ]

        ctx.Menu = { MenuId = 0x1022, TextId = 0x86, PrevId = 0, NextId = 0, BackId = 0x1010  }
        ctx.MenuLines[0] = { Type = LINE_TYPE.MENU, TextId = 0x190, ValId = 0x1023  }
        ctx.MenuLines[1] = { Type = LINE_TYPE.MENU, TextId = 0x80, ValId = 0x1025  }
        ctx.MenuLines[2] = { Type = LINE_TYPE.MENU, TextId = 0xAD, ValId = 0x1029 }
        ctx.MenuLines[3] = { Type = LINE_TYPE.MENU, TextId = 0xCA, ValId = 0x102A }
        ctx.MenuLines[4] = { Type = LINE_TYPE.MENU, TextId = 0x240, ValId = 0x1032 }
        ctx.SelLine = 0
        lastGoodMenu = menuId
    elseif (menuId==0x1023) then  
        --M[Id=0x1023 P=0x0 N=0x0 B=0x1022 Text="Relearn Servo Settings"[0x190]]
        --L[#3 T=M VId=0x1024 Text="Apply"[0x90]   MId=0x1023 ]
    
        ctx.Menu = { MenuId = 0x1023, TextId = 0x190, PrevId = 0, NextId = 0, BackId = 0x1022  }
        ctx.MenuLines[3] = { Type = LINE_TYPE.MENU, TextId = 0x90, ValId = 0x1024  }
        
        ctx.SelLine = 3
        lastGoodMenu = menuId
    elseif (menuId==0x1024) then  
        --M[Id=0x1024 P=0x0 N=0x0 B=0x0 Text="Relearn Servo Settings"[0x190]]
        --L[#3 T=M VId=0x1000 Text="Complete"[0x93]   MId=0x1024 ]

        ctx.Menu = { MenuId = 0x1024, TextId = 0x190, PrevId = 0, NextId = 0, BackId = 0  }
        ctx.MenuLines[3] = { Type = LINE_TYPE.MENU, TextId = 0x93, ValId = 0x1000  }
        
        ctx.SelLine = 3
        lastGoodMenu = menuId

    elseif (menuId==0x1025) then 
        --M[Id=0x1025 P=0x0 N=0x0 B=0x1022 Text="Orientation"[0x80]]
        --L[#0 T=M VId=0x1025 Text="Set the model level,"[0x21A]   MId=0x1025 ]
        --L[#1 T=M VId=0x1025 Text="and press Continue."[0x21B]   MId=0x1025 ]
        --L[#2 T=M VId=0x1025 Text=""[0x21C]   MId=0x1025 ]
        --L[#3 T=M VId=0x1025 Text=""[0x21D]   MId=0x1025 ]
        --L[#5 T=M VId=0x1026 Text="Continue"[0x224]   MId=0x1025 ]
        --L[#6 T=M VId=0x1027 Text="Set Orientation Manually"[0x229]   MId=0x1025 ]

        ctx.Menu = { MenuId = 0x1025, TextId = 0x80, PrevId = 0, NextId = 0, BackId = 0x1022 }
        ctx.MenuLines[0] = { Type = LINE_TYPE.MENU, TextId = 0x21A, ValId = 0x1025 }
        ctx.MenuLines[1] = { Type = LINE_TYPE.MENU, TextId = 0x21B, ValId = 0x1025 }
        ctx.MenuLines[2] = { Type = LINE_TYPE.MENU, TextId = 0x21C, ValId = 0x1025 }
        ctx.MenuLines[3] = { Type = LINE_TYPE.MENU, TextId = 0x21D, ValId = 0x1025 }
        ctx.MenuLines[5] = { Type = LINE_TYPE.MENU, TextId = 0x224, ValId = 0x1026 }
        ctx.MenuLines[6] = { Type = LINE_TYPE.MENU, TextId = 0x229, ValId = 0x1027 }
        ctx.SelLine = 5
        lastGoodMenu = menuId
    elseif (menuId==0x1026) then 
        --M[Id=0x1026 P=0x1025 N=0x0 B=0x1025 Text="Orientation"[0x80]]
        --L[#0 T=M VId=0x1026 Text="Set the model on its nose,"[0x21F]   MId=0x1026 ]
        --L[#1 T=M VId=0x1026 Text="and press Continue. If the"[0x220]   MId=0x1026 ]
        --L[#2 T=M VId=0x1026 Text="orientation on the next"[0x221]   MId=0x1026 ]
        --L[#3 T=M VId=0x1026 Text="screen is wrong go back"[0x222]   MId=0x1026 ]
        --L[#4 T=M VId=0x1026 Text="and try again."[0x223]   MId=0x1026 ]
        --L[#6 T=M VId=0x1027 Text="Continue"[0x224]   MId=0x1026 ]

        ctx.Menu = { MenuId = 0x1026, TextId = 0x80, PrevId = 0x1025, NextId = 0, BackId = 0x1025 }
        ctx.MenuLines[0] = { Type = LINE_TYPE.MENU, TextId = 0x21F, ValId = 0x1026 }
        ctx.MenuLines[1] = { Type = LINE_TYPE.MENU, TextId = 0x220, ValId = 0x1026 }
        ctx.MenuLines[2] = { Type = LINE_TYPE.MENU, TextId = 0x221, ValId = 0x1026 }
        ctx.MenuLines[3] = { Type = LINE_TYPE.MENU, TextId = 0x222, ValId = 0x1026 }
        ctx.MenuLines[4] = { Type = LINE_TYPE.MENU, TextId = 0x223, ValId = 0x1026 }
        ctx.MenuLines[6] = { Type = LINE_TYPE.MENU, TextId = 0x224, ValId = 0x1027 }

        ctx.SelLine = 6
        lastGoodMenu = menuId
    elseif (menuId==0x1027) then 
        --M[Id=0x1028 P=0x0 N=0x0 B=0x1028 Text="Orientation"[0x80]]
        --L[#5 T=LM_nc VId=0x1000 Text="Orientation"[0x80] Val=4|"RX Pos 5" NL=(0->23,0,S=203) [203->226,203] MId=0x1027 ]
        --L[#6 T=M VId=0x1028 Text="Continue"[0x224]   MId=0x1027 ]

        ctx.Menu = { MenuId = 0x1027, TextId = 0x80, PrevId = 0x1025, NextId = 0, BackId = 0x1025 }
        ctx.MenuLines[5] = { Type = LINE_TYPE.LIST_MENU_NC, TextId = 0x80, ValId = 0x1000, Min=203, Max=226, Def=203, Val=5 }
        ctx.MenuLines[6] = { Type = LINE_TYPE.MENU, TextId = 0x224, ValId = 0x1028 }
        ctx.SelLine = 5
        lastGoodMenu = menuId
    elseif (menuId==0x1028) then 
        --M[Id=0x1027 P=0x1025 N=0x0 B=0x1025 Text="Orientation"[0x80]]
        --L[#2 T=M VId=0x1 Text="Resetting RX... "[0x9F]   MId=0x1028 ]
        --L[#3 T=M VId=0x1028 Text="RX Pos 7"[0xD1]   MId=0x1028 ]

        ctx.Menu = { MenuId = 0x1028, TextId = 0x80, PrevId = 0x1025, NextId = 0, BackId = 0x1025 }
        ctx.MenuLines[2] = { Type = LINE_TYPE.MENU, TextId = 0x9F, ValId = 0x1 }
        ctx.MenuLines[3] = { Type = LINE_TYPE.MENU, TextId = 0xD1, ValId = 0x1028 }
        ctx.SelLine = 2
        lastGoodMenu = menuId

    elseif (menuId==0x1029) then  
        --M[Id=0x1029 P=0x0 N=0x0 B=0x1022 Text="Gain Channel Select"[0xAD]]
        --L[#0 T=M VId=0x1029 Text="AS3X"[0x1DC]   MId=0x1029 ]
        --L[#1 T=LM VId=0x1000 Text="Roll"[0x40] Val=0|"Inhibit?" NL=(0->32,0,S=53) [53->85,53] MId=0x1029 ]
        --L[#2 T=LM VId=0x1001 Text="Pitch"[0x41] Val=0|"Inhibit?" NL=(0->32,0,S=53) [53->85,53] MId=0x1029 ]
        --L[#3 T=LM VId=0x1002 Text="Yaw"[0x42] Val=0|"Inhibit?" NL=(0->32,0,S=53) [53->85,53] MId=0x1029 ]
        --L[#4 T=M VId=0x1029 Text="SAFE"[0xDA]   MId=0x1029 ]
        --L[#5 T=LM VId=0x1004 Text="Roll"[0x40] Val=0|"Inhibit?" NL=(0->32,0,S=53) [53->85,53] MId=0x1029 ]
        --L[#6 T=LM VId=0x1005 Text="Pitch"[0x41] Val=0|"Inhibit?" NL=(0->32,0,S=53) [53->85,53] MId=0x1029 ]

        ctx.Menu = { MenuId = 0x1029, TextId = 0xAD, PrevId = 0, NextId = 0, BackId = 0x1022  }
        ctx.MenuLines[0] = { Type = LINE_TYPE.MENU, TextId = 0x1DC, ValId = 0x1029  }

        ctx.MenuLines[1] = { Type = LINE_TYPE.LIST_MENU, TextId = 0x40, ValId = 0x1000, Min=53, Max=85, Def=53, Val=0 }
        ctx.MenuLines[2] = { Type = LINE_TYPE.LIST_MENU, TextId = 0x41, ValId = 0x1001, Min=53, Max=85, Def=53, Val=0 }
        ctx.MenuLines[3] = { Type = LINE_TYPE.LIST_MENU, TextId = 0x42, ValId = 0x1002, Min=53, Max=85, Def=53, Val=0 }
        ctx.MenuLines[4] = { Type = LINE_TYPE.MENU, TextId = 0xDA, ValId = 0x1029  }
        ctx.MenuLines[5] = { Type = LINE_TYPE.LIST_MENU, TextId = 0x40, ValId = 0x1004, Min=53, Max=85, Def=53, Val=0 }
        ctx.MenuLines[6] = { Type = LINE_TYPE.LIST_MENU, TextId = 0x41, ValId = 0x1005, Min=53, Max=85, Def=53, Val=0 }
   
        ctx.SelLine = 2
        lastGoodMenu = menuId
    elseif (menuId==0x1030) then  
        --M[Id=0x1030 P=0x0 N=0x0 B=0x102A Text="Attitude Trim"[0x1E6]]
        --L[#0 T=M VId=0x1030 Text="Level model and capture attiude/M"[0xCD]   MId=0x1030 ]  FORCED MENU
        --L[#1 T=M VId=0x1030 Text="Attitude Trim"[0x1E6]   MId=0x1030 ]
        --L[#2 T=V_de VId=0x1002 Text="Roll"[0x40] Val=-1 [-45->45,0] MId=0x1030 ]
        --L[#3 T=V_de VId=0x1003 Text="Pitch"[0x41] Val=7 [-45->45,0] MId=0x1030 ]
        --L[#5 T=M VId=0x1030 Text="Positive = Nose Up/Roll Right"[0x267]   MId=0x1030 ]
        --L[#6 T=M VId=0x1030 Text="Negative = Nose Down/Roll Left"[0x268]   MId=0x1030 ]

        ctx.Menu = { MenuId = 0x1030, TextId = 0x1E6, PrevId = 0, NextId = 0, BackId = 0x102A  }
        ctx.MenuLines[0] = { Type = LINE_TYPE.MENU, TextId = 0xCD, ValId = 0x1030  } -- FORCED MENU (/M)
        ctx.MenuLines[1] = { Type = LINE_TYPE.MENU, TextId = 0x1E6, ValId = 0x1030  }
        ctx.MenuLines[2] = { Type = LINE_TYPE.VALUE_DEGREES, TextId = 0x40, ValId = 0x1002, Min=-45, Max=45, Def=0, Val=-1 }
        ctx.MenuLines[3] = { Type = LINE_TYPE.VALUE_DEGREES, TextId = 0x41, ValId = 0x1003, Min=-45, Max=45, Def=0, Val=7 }
        ctx.MenuLines[5] = { Type = LINE_TYPE.MENU, TextId = 0x267, ValId = 0x1030  }
        ctx.MenuLines[6] = { Type = LINE_TYPE.MENU, TextId = 0x268, ValId = 0x1030  }
   
        ctx.SelLine = 0
        lastGoodMenu = menuId
    elseif (menuId==0x1031) then  
        --M[Id=0x1031 P=0x0 N=0x0 B=0x102A Text="Failsafe Angles"[0x1F6]]
        --L[#0 T=M VId=0x1031 Text="Failsafe Angles"[0x1F6]   MId=0x1031 ]
        --L[#1 T=V_de VId=0x1001 Text="Roll"[0x40] Val=0 [-90->90,0] MId=0x1031 ]
        --L[#2 T=V_de VId=0x1002 Text="Pitch"[0x41] Val=0 [-90->90,0] MId=0x1031 ]
        --L[#5 T=M VId=0x1031 Text="Positive = Nose Up/Roll Right"[0x267]   MId=0x1031 ]
        --L[#6 T=M VId=0x1031 Text="Negative = Nose Down/Roll Left"[0x268]   MId=0x1031 ]

        ctx.Menu = { MenuId = 0x1031, TextId = 0x1F6, PrevId = 0, NextId = 0, BackId = 0x102A  }
        ctx.MenuLines[0] = { Type = LINE_TYPE.MENU, TextId = 0x1F6, ValId = 0x1031  }
        ctx.MenuLines[1] = { Type = LINE_TYPE.VALUE_DEGREES, TextId = 0x40, ValId = 0x1001, Min=-90, Max=90, Def=0, Val=0 }
        ctx.MenuLines[2] = { Type = LINE_TYPE.VALUE_DEGREES, TextId = 0x41, ValId = 0x1002, Min=-90, Max=90, Def=0, Val=0 }
        ctx.MenuLines[5] = { Type = LINE_TYPE.MENU, TextId = 0x267, ValId = 0x1031  }
        ctx.MenuLines[6] = { Type = LINE_TYPE.MENU, TextId = 0x268, ValId = 0x1031  }
   
        ctx.SelLine = 1
        lastGoodMenu = menuId
    elseif (menuId==0x1032) then  
        --M[Id=0x1032 P=0x0 N=0x0 B=0x1022 Text="Utilities"[0x240]]
        --L[#0 T=M VId=0x1033 Text="Copy Flight Mode Settings"[0x23D]   MId=0x1032 ]
        --L[#1 T=LM_nc VId=0x1001 Text="Enabled F-Modes"[0x88] Val=2 NL=(0->9,2,S=182) [182->191,184] MId=0x1032 ]

        ctx.Menu = { MenuId = 0x1032, TextId = 0x240, PrevId = 0, NextId = 0, BackId = 0x1022  }
        ctx.MenuLines[0] = { Type = LINE_TYPE.MENU, TextId = 0x23D, ValId = 0x1033  }
        ctx.MenuLines[1] = { Type = LINE_TYPE.LIST_MENU_NC, TextId = 0x88, ValId = 0x1001, Min=182, Max=191, Def=184, Val=2 }
       
        ctx.SelLine = 0
        lastGoodMenu = menuId

    elseif (menuId==0x1033) then  
        --M[Id=0x1033 P=0x0 N=0x0 B=0x1032 Text="Copy Flight Mode Settings"[0x23D]]
        --L[#0 T=V_nc VId=0x1000 Text="Flight Mode"[0x8001] Val=0 [0->10,0] MId=0x1033 ]
        --L[#1 T=M VId=0x1033 Text="WARNING: "Target""[0x260]   MId=0x1033 ]
        --L[#2 T=M VId=0x1033 Text="flight mode will be overwritten"[0x261]   MId=0x1033 ]
        --L[#3 T=M VId=0x1033 Text="by "Source""[0x262]   MId=0x1033 ]
        --L[#4 T=LM_nc VId=0x1004 Text="Source Flight Mode"[0x23E] Val=0|"FM1" NL=(0->9,0,S=182) [182->191,182] MId=0x1033 ]
        --L[#5 T=LM_nc VId=0x1005 Text="Target Flight Mode"[0x23F] Val=0|"FM1" NL=(0->9,0,S=182) [182->191,182] MId=0x1033 ]
        --L[#6 T=M VId=0x1035 Text="Apply"[0x90]   MId=0x1034 ]


        ctx.Menu = { MenuId = 0x1033, TextId = 0x23D, PrevId = 0, NextId = 0, BackId = 0x1032  }
        ctx.MenuLines[0] = { Type = LINE_TYPE.VALUE_NUM_I8_NC, TextId = 0x8001, ValId = 0x1000, Min=0, Max=10, Def=0, Val=1 }
        ctx.MenuLines[1] = { Type = LINE_TYPE.MENU, TextId = 0x260, ValId = 0x1033  }
        ctx.MenuLines[2] = { Type = LINE_TYPE.MENU, TextId = 0x261, ValId = 0x1033  }
        ctx.MenuLines[3] = { Type = LINE_TYPE.MENU, TextId = 0x262, ValId = 0x1033  }
        ctx.MenuLines[4] = { Type = LINE_TYPE.LIST_MENU_NC, TextId = 0x23E, ValId = 0x1004, Min=182, Max=191, Def=182, Val=0 }
        ctx.MenuLines[5] = { Type = LINE_TYPE.LIST_MENU_NC, TextId = 0x23F, ValId = 0x1005, Min=182, Max=191, Def=182, Val=0 }
        ctx.MenuLines[6] = { Type = LINE_TYPE.MENU, TextId = 0x90, ValId = 0x1034  }
       
        ctx.SelLine = 4
        lastGoodMenu = menuId
    elseif (menuId==0x1034) then  
        --M[Id=0x1033 P=0x0 N=0x0 B=0x1032 Text="Copy Flight Mode Settings"[0x23D]]

        --L[#1 T=M VId=0x1033 Text="Are you sure you want to ovewrite the \"Target\""[0x251]   MId=0x1033 ]
        --L[#2 T=M VId=0x1033 Text="with the \"Source\" ? "[0x252]   MId=0x1033 ]
        --L[#3 T=M VId=0x1033 Text=""[0x253]   MId=0x1033 ]
        --L[#4 T=LM_nc VId=0x1004 Text="Source Flight Mode"[0x23E] Val=0|"FM1" NL=(0->0,0,S=182) [182->182,182] MId=0x1033 ]
        --L[#5 T=LM_nc VId=0x1005 Text="Target Flight Mode"[0x23F] Val=1|"FM2" NL=(0->0,0,S=182) [182->182,182] MId=0x1033 ]
        --L[#6 T=M VId=0x1035 Text="YES"[0x259]   MId=0x1034 ]


        ctx.Menu = { MenuId = 0x1034, TextId = 0x23D, PrevId = 0x1033, NextId = 0, BackId = 0x1033  }

        ctx.MenuLines[1] = { Type = LINE_TYPE.MENU, TextId = 0x251, ValId = 0x1034  }
        ctx.MenuLines[2] = { Type = LINE_TYPE.MENU, TextId = 0x252, ValId = 0x1034  }
        ctx.MenuLines[3] = { Type = LINE_TYPE.MENU, TextId = 0x253, ValId = 0x1034  }
        ctx.MenuLines[4] = { Type = LINE_TYPE.LIST_MENU_NC, TextId = 0x23E, ValId = 0x1004, Min=182, Max=182, Def=182, Val=0 }
        ctx.MenuLines[5] = { Type = LINE_TYPE.LIST_MENU_NC, TextId = 0x23F, ValId = 0x1005, Min=182, Max=182, Def=182, Val=1 }
        ctx.MenuLines[6] = { Type = LINE_TYPE.MENU, TextId = 0x259, ValId = 0x1035  }
       
        ctx.SelLine = 6
        lastGoodMenu = menuId
    elseif (menuId==0x1035) then 
        --M[Id=0x1035 P=0x0 N=0x0 B=0x1032 Text="Copy Flight Mode Settings"[0x23D]]
        --L[#3 T=M VId=0x1032 Text="Complete"[0x93]   MId=0x1035 ]

        ctx.Menu = { MenuId = 0x1035,  TextId = 0x23D, PrevId = 0, NextId = 0, BackId = 0x1032 }
        ctx.MenuLines[3] = { Type = LINE_TYPE.MENU,  TextId = 0x93,  ValId = 0x1032  }

        ctx.SelLine = 3
        lastGoodMenu = menuId
    elseif (menuId==0x102A) then  
        --M[Id=0x102A P=0x0 N=0x0 B=0x1022 Text="SAFE/Panic Mode Setup"[0xCA]]
        --L[#2 T=M VId=0x102B Text="Panic"[0x8B]   MId=0x102A ]
        --L[#3 T=M VId=0x102D Text="Throttle to Pitch"[0x1EE]   MId=0x102A ]
        --L[#4 T=M VId=0x1030 Text="Attitude Trim"[0x1E6]   MId=0x102A ]
        --L[#5 T=LM_nc VId=0x1006 Text="SAFE Failsafe FMode"[0x1FD] Val=0|"FM1" NL=(0->10,0,S=181) [181->191,181] MId=0x102A ]
        --L[#6 T=M VId=0x1031 Text="Failsafe Angles"[0x1F6]   MId=0x102A ]

        ctx.Menu = { MenuId = 0x102A, TextId = 0xCA, PrevId = 0, NextId = 0, BackId = 0x1022  }
        ctx.MenuLines[2] = { Type = LINE_TYPE.MENU, TextId = 0x8B, ValId = 0x102B  }
        ctx.MenuLines[3] = { Type = LINE_TYPE.MENU, TextId = 0x1EE, ValId = 0x102D  }
        ctx.MenuLines[4] = { Type = LINE_TYPE.MENU, TextId = 0x1E6, ValId = 0x1030  }
        ctx.MenuLines[5] = { Type = LINE_TYPE.LIST_MENU_NC, TextId = 0x1FD, ValId = 0x1006, Min=182, Max=191, Def=182, Val=0 }
        ctx.MenuLines[6] = { Type = LINE_TYPE.MENU, TextId = 0x1F6, ValId = 0x1031  }
       
        ctx.SelLine = 5
        lastGoodMenu = menuId
    elseif (menuId==0x102B) then  
        --M[Id=0x102B P=0x0 N=0x0 B=0x102A Text="Panic"[0x8B]]
        --L[#0 T=LM VId=0x1000 Text="Panic Channel"[0xD2] Val=0|"Inhibit?" NL=(0->32,0,S=53) [53->85,53] MId=0x102B ]
        --L[#1 T=LM VId=0x1001 Text="Panic Delay"[0x8E] Val=0|"Inh" NL=(0->1,0,S=3) [3->4,3] MId=0x102B ]
        --L[#2 T=LM VId=0x1002 Text="Panic Flight Mode"[0x1FC] Val=0|"FM1" NL=(0->10,0,S=181) [181->191,181] MId=0x102B ]
        --L[#3 T=V_nc VId=0x1003 Text="Roll Right"[0x1E9] Val=30 [0->90,30] MId=0x102B ]
        --L[#4 T=V_nc VId=0x1004 Text="Roll Left"[0x1EA] Val=30 [0->90,30] MId=0x102B ]
        --L[#5 T=V_nc VId=0x1005 Text="Pitch Down"[0x1EB] Val=30 [0->75,30] MId=0x102B ]
        --L[#6 T=V_nc VId=0x1006 Text="Pitch Up"[0x1EC] Val=30 [0->75,30] MId=0x102B ]

        ctx.Menu = { MenuId = 0x102B, TextId = 0x8B, PrevId = 0, NextId = 0, BackId = 0x102A  }
        ctx.MenuLines[0] = { Type = LINE_TYPE.LIST_MENU, TextId = 0xD2, ValId = 0x1000, Min=53, Max=85, Def=53, Val=0 }
        ctx.MenuLines[1] = { Type = LINE_TYPE.LIST_MENU, TextId = 0x8E, ValId = 0x1001, Min=3, Max=4, Def=3, Val=0 }
        ctx.MenuLines[2] = { Type = LINE_TYPE.LIST_MENU, TextId = 0x1FC, ValId = 0x1002, Min=181, Max=191, Def=181, Val=0 }
        ctx.MenuLines[3] = { Type = LINE_TYPE.VALUE_NUM_I8_NC, TextId = 0x1E9, ValId = 0x1003, Min=0, Max=90, Def=30, Val=30 }
        ctx.MenuLines[4] = { Type = LINE_TYPE.VALUE_NUM_I8_NC, TextId = 0x1EA, ValId = 0x1004, Min=0, Max=90, Def=30, Val=30 }
        ctx.MenuLines[5] = { Type = LINE_TYPE.VALUE_NUM_I8_NC, TextId = 0x1E9, ValId = 0x1005, Min=0, Max=75, Def=30, Val=30 }
        ctx.MenuLines[6] = { Type = LINE_TYPE.VALUE_NUM_I8_NC, TextId = 0x1EC, ValId = 0x1006, Min=0, Max=75, Def=30, Val=30 }

        ctx.SelLine = 0
        lastGoodMenu = menuId
    elseif (menuId==0x102D) then  
        --M[Id=0x102D P=0x0 N=0x0 B=0x102A Text="SAFE - Throttle to Pitch"[0x269]]
        --L[#0 T=M VId=0x102D Text="Postive = Up, Negative = Down"[0x254]   MId=0x102D ]
        --L[#1 T=M VId=0x102D Text="Low Thr to Pitch"[0x1EF]   MId=0x102D ]
        --L[#2 T=V_nc VId=0x1001 Text="Threshold"[0x1F3] Val=30 [0->50,30] MId=0x102D ]
        --L[#3 T=V_de VId=0x1002 Text="Angle"[0x1F4] Val=0 [-45->45,0] MId=0x102D ]
        --L[#4 T=M VId=0x102D Text="High Thr to Pitch"[0x1F0]   MId=0x102D ]
        --L[#5 T=V_nc VId=0x1005 Text="Threshold"[0x1F3] Val=70 [51->100,70] MId=0x102D ]
        --L[#6 T=V_de VId=0x1006 Text="Angle"[0x1F4] Val=0 [-45->45,0] MId=0x102D ]

        ctx.Menu = { MenuId = 0x102D, TextId = 0x269, PrevId = 0, NextId = 0, BackId = 0x102A  }
        ctx.MenuLines[0] = { Type = LINE_TYPE.MENU, TextId = 0x254, ValId = 0x102D  }
        ctx.MenuLines[1] = { Type = LINE_TYPE.MENU, TextId = 0x1EF, ValId = 0x102D  }
        ctx.MenuLines[2] = { Type = LINE_TYPE.VALUE_NUM_I8_NC, TextId = 0x1F3, ValId = 0x1001, Min=0, Max=50, Def=30, Val=30 }
        ctx.MenuLines[3] = { Type = LINE_TYPE.VALUE_DEGREES, TextId = 0x1F4, ValId = 0x1002, Min=-45, Max=45, Def=0, Val=0 }
        ctx.MenuLines[4] = { Type = LINE_TYPE.MENU, TextId = 0x1F0, ValId = 0x102D  }
        ctx.MenuLines[5] = { Type = LINE_TYPE.VALUE_NUM_I8_NC, TextId = 0x1F3, ValId = 0x1005, Min=51, Max=100, Def=70, Val=70 }
        ctx.MenuLines[6] = { Type = LINE_TYPE.VALUE_DEGREES, TextId = 0x1F4, ValId = 0x1006, Min=-45, Max=45, Def=0, Val=0 }

        ctx.SelLine = 2
        lastGoodMenu = menuId
    elseif (menuId==0x104F) then
        --M[Id=0x104F P=0x0 N=0x1050 B=0x1010 Text="First Time Setup"]
        --L[#0 T=M VId=0x104F Text="Make sure the model has been" MId=0x104F ]
        --L[#1 T=M VId=0x104F Text="configured, including wing type," MId=0x104F ]
        --L[#2 T=M VId=0x104F Text="reversing, travel, trimmed, etc." MId=0x104F ]
        --L[#3 T=M VId=0x104F [0->0,2] Text="before continuing setup." MId=0x104F ]
        --L[#4 T=M VId=0x104F [0->0,2] Text="" MId=0x104F ]
        --L[#5 T=M VId=0x104F [0->0,2] Text="" MId=0x104F ]

        ctx.Menu = { MenuId = 0x104F, TextId = 0x00F9, PrevId = 0, NextId = 0x1050, BackId = 0x1010 }
        ctx.MenuLines[0] = { Type = LINE_TYPE.MENU, TextId = 0x0100, ValId = 0x104F }
        ctx.MenuLines[1] = { Type = LINE_TYPE.MENU, TextId = 0x0101, ValId = 0x104F }
        ctx.MenuLines[2] = { Type = LINE_TYPE.MENU, TextId = 0x0102, ValId = 0x104F }
        ctx.MenuLines[3] = { Type = LINE_TYPE.MENU, TextId = 0x0103, ValId = 0x104F }
        ctx.MenuLines[4] = { Type = LINE_TYPE.MENU, TextId = 0x0104, ValId = 0x104F }
        ctx.SelLine = menuLib.NEXT_BUTTON
        lastGoodMenu = menuId
    elseif (menuId==0x1050) then  

        --M[Id=0x1050 P=0x104F N=0x1051 B=0x1010 Text="First Time Setup"]
        --L[#0 T=M VId=0x1050 Text="Any wing type, channel assignment," MId=0x1050 ]
        --L[#1 T=M VId=0x1050 Text="subtrim, or servo reversing changes" MId=0x1050 
        --L[#2 T=M VId=0x1050 Text="require running through initial" MId=0x1050 ]
        --L[#3 T=M VId=0x1050 Text="setup again." MId=0x1050 ]
    
        ctx.Menu = { MenuId = 0x1050, TextId = 0x00F9, PrevId = 0x104F, NextId = 0x1051, BackId = 0x1010 }
        ctx.MenuLines[0] = { Type = LINE_TYPE.MENU, TextId = 0x0106, ValId = 0x1050 }
        ctx.MenuLines[1] = { Type = LINE_TYPE.MENU, TextId = 0x0107, ValId = 0x1050 }
        ctx.MenuLines[2] = { Type = LINE_TYPE.MENU, TextId = 0x0108, ValId = 0x1050 }
        ctx.MenuLines[3] = { Type = LINE_TYPE.MENU, TextId = 0x0109, ValId = 0x1050 }
        ctx.MenuLines[4] = { Type = LINE_TYPE.MENU, TextId = 0x010A, ValId = 0x1050 }
        ctx.SelLine = menuLib.NEXT_BUTTON
        lastGoodMenu = menuId
    elseif (menuId==0x1051) then 
        --M[Id=0x1051 P=0x0 N=0x0 B=0x1010 Text="First Time Setup"]
        --L[#0 T=M VId=0x1051 Text="Set the model level," MId=0x1051 ]
        --L[#1 T=M VId=0x1051 Text="and press Continue." MId=0x1051 ]
        --L[#2 T=M VId=0x1051 Text="" MId=0x1051 ]
        --L[#5 T=M VId=0x1052 Text="Continue" MId=0x1051 ]
        --L[#6 T=M VId=0x1053 Text="Set Orientation Manually" MId=0x1051 ]

        ctx.Menu = { MenuId = 0x1051, TextId = 0x00F9, PrevId = 0, NextId = 0, BackId = 0x1010 }
        ctx.MenuLines[0] = { Type = LINE_TYPE.MENU, TextId = 0x021A, ValId = 0x1051 }
        ctx.MenuLines[1] = { Type = LINE_TYPE.MENU, TextId = 0x021B, ValId = 0x1051 }
        ctx.MenuLines[2] = { Type = LINE_TYPE.MENU, TextId = 0x021C, ValId = 0x1051 }
        ctx.MenuLines[5] = { Type = LINE_TYPE.MENU, TextId = 0x0224, ValId = 0x1052 }
        ctx.MenuLines[6] = { Type = LINE_TYPE.MENU, TextId = 0x0229, ValId = 0x1053 }

        ctx.SelLine = 5
        lastGoodMenu = menuId
    elseif (menuId==0x1052) then 
        --M[Id=0x1052 P=0x1051 N=0x0 B=0x1010 Text="First Time Setup"[0xA5]]
        --L[#0 T=M VId=0x1052 Text="Set the model on its nose,"[0x21F] MId=0x1052 ]
        --L[#1 T=M VId=0x1052 Text="and press Continue. If the"[0x220] MId=0x1052 ]
        --L[#2 T=M VId=0x1052 Text="orientation on the next"[0x221] MId=0x1052 ]
        --L[#3 T=M VId=0x1052 Text="screen is wrong go back"[0x222] MId=0x1052 ]
        --L[#4 T=M VId=0x1052 Text="and try again."[0x223] MId=0x1052 ]
        --L[#6 T=M VId=0x1053 Text="Continue"[0x224] MId=0x1052 ]

        ctx.Menu = { MenuId = 0x1052, TextId = 0x00A5, PrevId = 0x1051, NextId = 0, BackId = 0x1010 }
        ctx.MenuLines[0] = { Type = LINE_TYPE.MENU, TextId = 0x21F, ValId = 0x1052 }
        ctx.MenuLines[1] = { Type = LINE_TYPE.MENU, TextId = 0x220, ValId = 0x1052 }
        ctx.MenuLines[2] = { Type = LINE_TYPE.MENU, TextId = 0x221, ValId = 0x1052 }
        ctx.MenuLines[3] = { Type = LINE_TYPE.MENU, TextId = 0x222, ValId = 0x1052 }
        ctx.MenuLines[4] = { Type = LINE_TYPE.MENU, TextId = 0x223, ValId = 0x1052 }
        ctx.MenuLines[6] = { Type = LINE_TYPE.MENU, TextId = 0x224, ValId = 0x1053 }
        ctx.SelLine = 6
        lastGoodMenu = menuId
    elseif (menuId==0x1053) then 
        --M[Id=0x1053 P=0x1051 N=0x0 B=0x1010 Text="First Time Setup"[0xA5]]
        --L[#5 T=LM_nc VId=0x1000 Text="Orientation"[0x80] MId=0x1053 val=0 (0->23,0,S=203) [203->226,203] ]
        --L[#6 T=M VId=0x1054 Text="Continue"[0x224] MId=0x1053 ]
        
        ctx.Menu = { MenuId = 0x1053, TextId = 0x00A5, PrevId = 0x1051, NextId = 0, BackId = 0x1010 }
        ctx.MenuLines[5] = { Type = LINE_TYPE.LIST_MENU_NC, TextId = 0x80, ValId = 0x1000, Min=203, Max=226, Def=203, Val=0 }
        ctx.MenuLines[6] = { Type = LINE_TYPE.MENU, TextId = 0x224, ValId = 0x1054 }
        ctx.SelLine = 5
        lastGoodMenu = menuId
    elseif (menuId==0x1054) then 
        --M[Id=0x1054 P=0x1053 N=0x0 B=0x1010 Text="First Time Setup"[0xA5]]
        --L[#1 T=M VId=0x7CA5 Text="Gain Channel Select"[0xAD] ]
        --L[#6 T=M VId=0x1 Text="Apply"[0x90] MId=0x1054 ]

        ctx.Menu = { MenuId = 0x1054, TextId = 0x00A5, PrevId = 0x1053, NextId = 0, BackId = 0x1010 }
        ctx.MenuLines[1] = { Type = LINE_TYPE.MENU, TextId = 0xAD, ValId = 0x7CA5  }
        ctx.MenuLines[6] = { Type = LINE_TYPE.MENU, TextId = 0x90, ValId = 0x01 } -- Special save&reboot??
        ctx.SelLine = 1
        lastGoodMenu = menuId
    elseif (menuId==0x1055) then 
        --M[Id=0x1055 P=0x0 N=0x0 B=0x1010 Text="First Time SAFE Setup"[0x20D]]
        --L[#0 T=M VId=0x1055 Text="Before setting up SAFE"[0x255]   MId=0x1055 ]
        --L[#1 T=M VId=0x1055 Text="a Fligt Mode channel"[0x256]   MId=0x1055 ]
        --L[#2 T=M VId=0x1055 Text="most be configured."[0x257]   MId=0x1055 ]
        --L[#5 T=M VId=0x7CA7 Text="FM Channel"[0x78]   MId=0x1055 ]
        --L[#6 T=M VId=0x1056 Text="Continue"[0x224]   MId=0x1055 ]

        ctx.Menu = { MenuId = 0x1055, TextId = 0x20D, PrevId = 0x1053, NextId = 0, BackId = 0x1010 }
        ctx.MenuLines[0] = { Type = LINE_TYPE.MENU, TextId = 0x255, ValId = 0x1055  }
        ctx.MenuLines[1] = { Type = LINE_TYPE.MENU, TextId = 0x256, ValId = 0x1055 } 
        ctx.MenuLines[2] = { Type = LINE_TYPE.MENU, TextId = 0x257, ValId = 0x1055 } 
        ctx.MenuLines[5] = { Type = LINE_TYPE.MENU, TextId = 0x78, ValId = 0x7CA7 } 
        ctx.MenuLines[6] = { Type = LINE_TYPE.MENU, TextId = 0x224, ValId = 0x1056 } 
        ctx.SelLine = 5
        lastGoodMenu = menuId
    elseif (menuId==0x1056) then 
        --M[Id=0x1056 P=0x1055 N=0x1057 B=0x1010 Text="First Time SAFE Setup"[0x20D]]
        --L[#0 T=V_nc VId=0x1000 Text="Flight Mode"[0x8001] Val=1 [0->10,0] MId=0x1056 ]
        --L[#1 T=M VId=0x1056 Text="Select the desired flight mode"[0x25A]   MId=0x1056 ]
        --L[#2 T=M VId=0x1056 Text="switch position to adjust settings"[0x25B]   MId=0x1056 ]
        --L[#3 T=M VId=0x1056 Text="for each flight mode"[0x25C]   MId=0x1056 ]
        --L[#4 T=M VId=0x1056 Text=""[0x25D]   MId=0x1056 ]
        --L[#5 T=M VId=0x1056 Text=""[0x25E]   MId=0x1056 ]

        ctx.Menu = { MenuId = 0x1056, TextId = 0x20D, PrevId = 0x1053, NextId = 0x1057, BackId = 0x1010 }
        ctx.MenuLines[0] = { Type = LINE_TYPE.VALUE_NUM_I8_NC, TextId = 0x8001, ValId = 0x1000, Min=0, Max=10, Def=0, Val=1 }
        ctx.MenuLines[1] = { Type = LINE_TYPE.MENU, TextId = 0x25A, ValId = 0x1056 } 
        ctx.MenuLines[2] = { Type = LINE_TYPE.MENU, TextId = 0x25B, ValId = 0x1056 } 
        ctx.MenuLines[3] = { Type = LINE_TYPE.MENU, TextId = 0x25C, ValId = 0x1056 } 
        ctx.MenuLines[4] = { Type = LINE_TYPE.MENU, TextId = 0x25D, ValId = 0x1056 } 
        ctx.MenuLines[5] = { Type = LINE_TYPE.MENU, TextId = 0x25E, ValId = 0x1056 } 

        ctx.SelLine = 7
        lastGoodMenu = menuId
    elseif (menuId==0x1057) then 
        --M[Id=0x1057 P=0x1056 N=0x1059 B=0x1010 Text="First Time SAFE Setup"[0x20D]]
        --L[#0 T=M VId=0x1057 Text="AS3X gains must be tuned"[0x20E]   MId=0x1057 ]
        --L[#1 T=M VId=0x1057 Text="and active i SAFE Flight Modes"[0x20F]   MId=0x1057 ]
        --L[#2 T=M VId=0x1057 Text="to help reduce wobble."[0x210]   MId=0x1057 ]
        --L[#3 T=M VId=0x1057 Text=""[0x211]   MId=0x1057 ]
        --L[#4 T=M VId=0x1057 Text=""[0x212]   MId=0x1057 ]
        --L[#5 T=M VId=0x1057 Text=""[0x213]   MId=0x1057 ]

        ctx.Menu = { MenuId = 0x1057, TextId = 0x20D, PrevId = 0x1056, NextId = 0x1059, BackId = 0x1010 }
        ctx.MenuLines[0] = { Type = LINE_TYPE.MENU, TextId = 0x20E, ValId = 0x1057 } 
        ctx.MenuLines[1] = { Type = LINE_TYPE.MENU, TextId = 0x20F, ValId = 0x1057 } 
        ctx.MenuLines[2] = { Type = LINE_TYPE.MENU, TextId = 0x210, ValId = 0x1057 } 
        ctx.MenuLines[3] = { Type = LINE_TYPE.MENU, TextId = 0x211, ValId = 0x1057 } 
        ctx.MenuLines[4] = { Type = LINE_TYPE.MENU, TextId = 0x212, ValId = 0x1057 } 
        ctx.MenuLines[5] = { Type = LINE_TYPE.MENU, TextId = 0x213, ValId = 0x1057 } 
        
        ctx.SelLine = 7
        lastGoodMenu = menuId

    elseif (menuId==0x1059) then 
        --M[Id=0x1059 P=0x1057 N=0x105A B=0x1010 Text="First Time SAFE Setup"[0x20D]]
        --L[#0 T=M VId=0x1059 Text="Leven model and capture attiude"[0xCD]   MId=0x1059 ]
        --L[#1 T=M VId=0x1059 Text="Attitude Trim"[0x1E6]   MId=0x1059 ]
        --L[#2 T=V_de VId=0x1002 Text="Roll"[0x40] Val=13 [-45->45,0] MId=0x1059 ]
        --L[#3 T=V_de VId=0x1003 Text="Pitch"[0x41] Val=5 [-45->45,0] MId=0x1059 ]
        --L[#5 T=M VId=0x1059 Text="Positive = Nose Up/Roll Right"[0x267]   MId=0x1059 ]
        --L[#6 T=M VId=0x1059 Text="Negative = Nose Down/Roll Left"[0x268]   MId=0x1059 ]

        ctx.Menu = { MenuId = 0x1059, TextId = 0x20D, PrevId = 0x1057, NextId = 0x105A, BackId = 0x1010 }
        ctx.MenuLines[0] = { Type = LINE_TYPE.MENU, TextId = 0xCD, ValId = 0x1059 } 
        ctx.MenuLines[1] = { Type = LINE_TYPE.MENU, TextId = 0x1E6, ValId = 0x1059 } 
        ctx.MenuLines[2] = { Type = LINE_TYPE.VALUE_DEGREES, TextId = 0x40, ValId = 0x1002, Min=-45, Max=45, Def=0, Val=13 } 
        ctx.MenuLines[3] = { Type = LINE_TYPE.VALUE_DEGREES, TextId = 0x41, ValId = 0x1003, Min=-45, Max=45, Def=0, Val=5 } 
        ctx.MenuLines[5] = { Type = LINE_TYPE.MENU, TextId = 0x267, ValId = 0x1059 } 
        ctx.MenuLines[6] = { Type = LINE_TYPE.MENU, TextId = 0x268, ValId = 0x1059 } 
        
        ctx.SelLine = 7
        lastGoodMenu = menuId
    elseif (menuId==0x105A) then
        --M[Id=0x105A P=0x1059 N=0x105B B=0x1010 Text="First Time SAFE Setup"[0x20D]]
        --L[#0 T=V_nc VId=0x1000 Text="Flight Mode"[0x8001] Val=0 [0->10,0] MId=0x105A ]
        --L[#1 T=LM VId=0x1001 Text="Safe Mode"[0x1F8] Val=3|"Inh" NL=(0->244,3,S=0) [0->244,3] MId=0x105A ]
        --L[#2 T=M VId=0x105A Text="Angle Limits "[0x226]   MId=0x105A ]
        --L[#3 T=V_nc VId=0x1003 Text="Roll Right"[0x1E9] Val=60 [10->90,60] MId=0x105A ]
        --L[#4 T=V_nc VId=0x1004 Text="Roll Left"[0x1EA] Val=60 [10->90,60] MId=0x105A ]
        --L[#5 T=V_nc VId=0x1005 Text="Pitch Down"[0x1EB] Val=40 [10->75,40] MId=0x105A ]
        --L[#6 T=V_nc VId=0x1006 Text="Pitch Up"[0x1EC] Val=50 [10->75,50] MId=0x105A ]
        
        ctx.Menu = { MenuId = 0x105A, TextId = 0x1DE, PrevId = 0x1059, NextId = 0x105B, BackId = 0x1010 }
        ctx.MenuLines[0] = { Type = LINE_TYPE.VALUE_NUM_I8_NC, TextId = 0x8001, ValId = 0x1000, Min=0, Max=10, Def=0, Val=1 }
        ctx.MenuLines[1] = { Type = LINE_TYPE.LIST_MENU, TextId = 0x1F8, ValId = 0x1001, Min=0, Max=244, Def=3, Val=3 }
        ctx.MenuLines[2] = { Type = LINE_TYPE.MENU, TextId = 0x226, ValId = 0x105A }
        ctx.MenuLines[3] = { Type = LINE_TYPE.VALUE_NUM_I8_NC, TextId = 0x1E9, ValId = 0x1003, Min=10, Max=90, Def=60, Val=60  }
        ctx.MenuLines[4] = { Type = LINE_TYPE.VALUE_NUM_I8_NC, TextId = 0x1EA, ValId = 0x1004, Min=10, Max=90, Def=60, Val=60  }
        ctx.MenuLines[5] = { Type = LINE_TYPE.VALUE_NUM_I8_NC, TextId = 0x1EB, ValId = 0x1004, Min=10, Max=75, Def=40, Val=40  }
        ctx.MenuLines[6] = { lineNum = 7, Type = LINE_TYPE.VALUE_NUM_I8_NC, TextId = 0x1EC, ValId = 0x1004, Min=10, Max=75, Def=50, Val=50  }

        ctx.SelLine = 1
        lastGoodMenu = menuId
    elseif (menuId==0x105B) then 
        --M[Id=0x105B P=0x105A N=0x0 B=0x1000 Text="First Time SAFE Setup"[0x20D]]
        --L[#3 T=M VId=0x1 Text="Apply"[0x90]   MId=0x1064 ]

        ctx.Menu = { MenuId = 0x105B,  TextId = 0x20D, PrevId = 0x105A, NextId = 0, BackId = 0x1000 }
        ctx.MenuLines[3] = { Type = LINE_TYPE.MENU,  TextId = 0x90,  ValId = 0x1  } -- reset RX 

        ctx.SelLine = 3
        lastGoodMenu = menuId
    elseif (menuId==0x105C) then
        -- M[Id=0x105C P=0x0 N=0x0 B=0x1010 Text="SAFE Select"[0x1F9]]
        --L[#0 T=V_nc VId=0x1000 Text="Flight Mode 1"[0x8001] val=1 [0->10,0] MId=0x105C ]
        --L[#1 T=LM VId=0x1001 Text="SAFE Select Channel"[0x1D7] val=5 NL=(0->32,53,S=53) [53->85,53] MId=0x105C]
        --L[#2 T=LM VId=0x1002 Text="AS3X"[0x1DC]  val=1 NL=(0->1,1,S=1) [1->2,1] MId=0x105C]
        --L[#3 T=LM VId=0x1003 Text="SAFE"[0xDA] val=0 NL=(0->0,0,S=1) [1->1,1] MId=0x105C ]
        --L[#6 T=LM VId=0x1006 Text="SAFE Select"[0x1F9] val=0 NL=(0->1,1,S=1) [1->2,1]  MId=0x105C ]

        ctx.Menu = { MenuId = 0x105C, TextId = 0x01F9, PrevId = 0, NextId = 0, BackId = 0x1010 }
        ctx.MenuLines[0] = { Type = LINE_TYPE.VALUE_NUM_I8_NC, TextId = 0x8001, ValId = 0x1000, Min=0, Max=10, Def=0, Val=1 }
        ctx.MenuLines[1] = { Type = LINE_TYPE.LIST_MENU, TextId = 0x1D7, ValId = 0x1001, Min=53, Max=85, Def=53, Val=5 }
        ctx.MenuLines[2] = { Type = LINE_TYPE.LIST_MENU, TextId = 0x1DC, ValId = 0x1002, Min=1, Max=2, Def=1, Val=1  }
        ctx.MenuLines[3] = { Type = LINE_TYPE.LIST_MENU, TextId = 0xDA, ValId = 0x1003, Min=1, Max=1, Def=1, Val=0  }
        ctx.MenuLines[6] = { Type = LINE_TYPE.LIST_MENU, TextId = 0x1F9, ValId = 0x1004, Min=1, Max=2, Def=1, Val=0  }

        ctx.SelLine = 1
        lastGoodMenu = menuId
    elseif (menuId==0x105E) then 
        -- M[Id=0x105E P=0x0 N=0x0 B=0x1000 Text="Other settings"]
        -- L[#1 T=M VId=0x1060 Text="Failsafe" MId=0x105E ]
        -- L[#2 T=M VId=0x1064 Text="Enter Receiver Bind Mode" MId=0x105E ]
        -- L[#3 T=M VId=0x1065 Text="Frame Rate" MId=0x105E ]
        -- L[#4 T=M VId=0x1067 Text="Factory Reset" MId=0x105E ]
        -- L[#5 T=M VId=0x1069 Text="Restore from Backup" MId=0x105E ]
        -- L[#6 T=M VId=0x106A Text="Save to Backup" MId=0x105E ]

        ctx.Menu = { MenuId = 0x105E, TextId = 0x0227, PrevId = 0, NextId = 0, BackId = 0x1010 }
        ctx.MenuLines[1] = { Type = LINE_TYPE.MENU, TextId = 0x004A, ValId = 0x1060 }
        ctx.MenuLines[2] = { Type = LINE_TYPE.MENU, TextId = 0x019C, ValId = 0x1064 }
        ctx.MenuLines[3] = { Type = LINE_TYPE.MENU, TextId = 0x0085, ValId = 0x1065 }
        ctx.MenuLines[4] = { Type = LINE_TYPE.MENU, TextId = 0x0097, ValId = 0x1067 }
        ctx.MenuLines[5] = { Type = LINE_TYPE.MENU, TextId = 0x020A, ValId = 0x1069 }
        ctx.MenuLines[6] = { Type = LINE_TYPE.MENU, TextId = 0x0209, ValId = 0x106A }
 
        ctx.SelLine = 1
        lastGoodMenu = menuId
    elseif (menuId==0x1060) then 
        --M[Id=0x1060 P=0x0 N=0x0 B=0x105E Text="Failsafe"]
        --L[#0 T=M VId=0x1061 Text="Failsafe" MId=0x1060 ]
        --L[#1 T=M VId=0x1062 Text="Capture Failsafe Positions" MId=0x1060 ]

        ctx.Menu = { MenuId = 0x1060, TextId = 0x004A, PrevId = 0, NextId = 0, BackId = 0x105E }
        ctx.MenuLines[0] = { Type = LINE_TYPE.MENU,  TextId = 0x004A, ValId = 0x1061 }
        ctx.MenuLines[1] = { Type = LINE_TYPE.MENU,  TextId = 0x009A, ValId = 0x1062 }
        ctx.SelLine = 0
        lastGoodMenu = menuId
    elseif (menuId==0x1061) then 
        --M[Id=0x1061 P=0x0 N=0x0 B=0x1060 Text="Failsafe"]
        --L[#0 T=LM_nc VId=0x1000 Text="Outputs:" val=0 NL=(0->19,54,S=54) [54->73,54] MId=0x1061 ]
        --L[#2 T=LM_tog VId=0x1002 Text="Custom Failsafe:" val=0 NL=(0->1,95,S=95) [95->96,95]  MId=0x1061 ]
        --L[#3 T=V_% VId=0x1003  Text="Position:" val=-100 [-150->150,0] MId=0x1061 ]

        ctx.Menu = { MenuId = 0x1061,  TextId = 0x004A, PrevId = 0, NextId = 0, BackId = 0x1060 }
        ctx.MenuLines[0] = { Type = LINE_TYPE.LIST_MENU_NC,  TextId = 0x0050, ValId = 0x1000, Min=54, Max=73, Def=54, Val=0 }
        ctx.MenuLines[1] = { Type = LINE_TYPE.LIST_MENU_TOG,    TextId = 0x009C, ValId = 0x1002, Min=95, Max=96, Def=95, Val=0 }
        ctx.MenuLines[2] = { Type = LINE_TYPE.VALUE_PERCENT, TextId = 0x004E, ValId = 0x1002, Min=-150, Max=150, Def=0, Val=-100 }
        ctx.SelLine = 0
        lastGoodMenu = menuId
    elseif (menuId==0x1064) then 
        --M[Id=0x1064 P=0x0 N=0x0 B=0x105E Text="Enter Receiver Bind Mode"[0x19C]]
        --L[#3 T=M VId=0x1 Text="Apply"[0x90]   MId=0x1064 ]

        ctx.Menu = { MenuId = 0x1064,  TextId = 0x19C, PrevId = 0, NextId = 0, BackId = 0x105E }
        ctx.MenuLines[3] = { Type = LINE_TYPE.MENU,  TextId = 0x90,  ValId = 0x1  } -- reset RX 

        ctx.SelLine = 3
        lastGoodMenu = menuId
    elseif (menuId==0x1065) then 
        --M[Id=0x1065 P=0x0 N=0x0 B=0x105E Text="Frame Rate"]
        --L[#0 T=LM VId=0x1000 Text="Output Channel 1:" val=46 NL=(0->244,46|S=0) [0->244,0]  MId=0x1065 ]
        --L[#1 T=LM VId=0x1001 Text="Output Channel 2:" val=47 NL=(0->244,46|S=0) [0->244,0]  MId=0x1065 ]
        --L[#2 T=LM VId=0x1002 Text="Output Channel 3:" val=46 NL=(0->244,46|S=0) [0->244,0]  MId=0x1065 ]
        --L[#3 T=LM VId=0x1003 Text="Output Channel 4:" val=46 NL=(0->244,46|S=0) [0->244,0]  MId=0x1065 ]
        --L[#4 T=LM VId=0x1004 Text="Output Channel 5:" val=46 NL=(0->244,46|S=0) [0->244,0]  MId=0x1065 ]
        --L[#5 T=LM VId=0x1005 Text="Output Channel 6:" val=46 NL=(0->244,46|S=0) [0->244,0]  MId=0x1065 ]

        ctx.Menu = { MenuId = 0x1065, TextId = 0x0085, PrevId = 0, NextId = 0, BackId = 0x105E }
        ctx.MenuLines[0] = { Type = LINE_TYPE.LIST_MENU, TextId = 0x0051, ValId = 0x1000, Min=0, Max=244, Def=46, Val=46 }
        ctx.MenuLines[1] = { Type = LINE_TYPE.LIST_MENU, TextId = 0x0052, ValId = 0x1001, Min=0, Max=244, Def=46, Val=47 }
        ctx.MenuLines[2] = { Type = LINE_TYPE.LIST_MENU, TextId = 0x0053, ValId = 0x1002, Min=0, Max=244, Def=46, Val=46 }
        ctx.MenuLines[3] = { Type = LINE_TYPE.LIST_MENU, TextId = 0x0054, ValId = 0x1002, Min=0, Max=244, Def=46, Val=46 }
        ctx.MenuLines[4] = { Type = LINE_TYPE.LIST_MENU, TextId = 0x0055, ValId = 0x1002, Min=0, Max=244, Def=46, Val=46 }
        ctx.MenuLines[5] = { Type = LINE_TYPE.LIST_MENU, TextId = 0x0056, ValId = 0x1002, Min=0, Max=244, Def=46, Val=46 }

        ctx.SelLine = 0
        lastGoodMenu = menuId
    elseif (menuId==0x1067) then 
        --M[Id=0x1067 P=0x0 N=0x0 B=0x105E Text="WARNING!"[0x22B]]
        --L[#0 T=M VId=0x1067 Text="This will reset the"[0x22C]   MId=0x1067 ]
        --L[#1 T=M VId=0x1067 Text="configuration to factory"[0x22D]   MId=0x1067 ]
        --L[#2 T=M VId=0x1067 Text="defaults. This does not"[0x22E]   MId=0x1067 ]
        --L[#3 T=M VId=0x1067 Text="affect the backup config."[0x22F]   MId=0x1067 ]
        --L[#4 T=M VId=0x1067 Text=""[0x230]   MId=0x1067 ]
        --L[#6 T=M VId=0x1068 Text="Apply"[0x90]   MId=0x1067 ]

        ctx.Menu = { MenuId = 0x1067,  TextId = 0x22B, PrevId = 0, NextId = 0, BackId = 0x105E }
        ctx.MenuLines[0] = { Type = LINE_TYPE.MENU,  TextId = 0x22C,  ValId = 0x1067  }
        ctx.MenuLines[1] = { Type = LINE_TYPE.MENU,  TextId = 0x22D,  ValId = 0x1067  }
        ctx.MenuLines[2] = { Type = LINE_TYPE.MENU,  TextId = 0x22E,  ValId = 0x1067  }
        ctx.MenuLines[3] = { Type = LINE_TYPE.MENU,  TextId = 0x22F,  ValId = 0x1067  }
        ctx.MenuLines[4] = { Type = LINE_TYPE.MENU,  TextId = 0x230,  ValId = 0x1067  }
        ctx.MenuLines[6] = { Type = LINE_TYPE.MENU,  TextId = 0x90,  ValId = 0x1068  }

        ctx.SelLine = 6
        lastGoodMenu = menuId
    elseif (menuId==0x1068) then 
        --M[Id=0x1068 P=0x0 N=0x0 B=0x1000 Text="Done"[0x94]]
        --L[#3 T=M VId=0x1000 Text="Complete"[0x93]   MId=0x1068 ]

        ctx.Menu = { MenuId = 0x1068,  TextId = 0x94, PrevId = 0, NextId = 0, BackId = 0x1000 }
        ctx.MenuLines[3] = { Type = LINE_TYPE.MENU,  TextId = 0x93,  ValId = 0x1000  }

        ctx.SelLine = 3
        lastGoodMenu = menuId
    elseif (menuId==0x1069) then 
        --M[Id=0x1069 P=0x0 N=0x0 B=0x105E Text="WARNING!"[0x22B]]
        --L[#0 T=M VId=0x1069 Text="This will overwrite the"[0x236]   MId=0x1069 ]
        --L[#1 T=M VId=0x1069 Text="current config with"[0x237]   MId=0x1069 ]
        --L[#2 T=M VId=0x1069 Text="that which is in"[0x238]   MId=0x1069 ]
        --L[#3 T=M VId=0x1069 Text="the backup memory."[0x239]   MId=0x1069 ]
        --L[#4 T=M VId=0x1069 Text=""[0x23A]   MId=0x1069 ]
        --L[#6 T=M VId=0x1068 Text="Apply"[0x90]   MId=0x1069 ]

        ctx.Menu = { MenuId = 0x1069,  TextId = 0x22B, PrevId = 0, NextId = 0, BackId = 0x105E }
        ctx.MenuLines[0] = { Type = LINE_TYPE.MENU,  TextId = 0x236,  ValId = 0x1069  }
        ctx.MenuLines[1] = { Type = LINE_TYPE.MENU,  TextId = 0x237,  ValId = 0x1069  }
        ctx.MenuLines[2] = { Type = LINE_TYPE.MENU,  TextId = 0x238,  ValId = 0x1069  }
        ctx.MenuLines[3] = { Type = LINE_TYPE.MENU,  TextId = 0x239,  ValId = 0x1069  }
        ctx.MenuLines[4] = { Type = LINE_TYPE.MENU,  TextId = 0x23A,  ValId = 0x1069  }
        ctx.MenuLines[6] = { Type = LINE_TYPE.MENU,  TextId = 0x90,  ValId = 0x1068  }

        ctx.SelLine = 6
        lastGoodMenu = menuId
    elseif (menuId==0x106A) then 
        --M[Id=0x106A P=0x0 N=0x0 B=0x105E Text="WARNING!"[0x22B]]
        --L[#0 T=M VId=0x106A Text="This will overwrite the"[0x231]   MId=0x106A ]
        --L[#1 T=M VId=0x106A Text="backup memory with your"[0x232]   MId=0x106A ]
        --L[#2 T=M VId=0x106A Text="current configuartion."[0x233]   MId=0x106A ]
        --L[#3 T=M VId=0x106A Text=""[0x234]   MId=0x106A ]
        --L[#4 T=M VId=0x106A Text=""[0x235]   MId=0x106A ]
        --L[#6 T=M VId=0x1068 Text="Apply"[0x90]   MId=0x106A ]

        ctx.Menu = { MenuId = 0x106A,  TextId = 0x22B, PrevId = 0, NextId = 0, BackId = 0x105E }
        ctx.MenuLines[0] = { Type = LINE_TYPE.MENU,  TextId = 0x231,  ValId = 0x106A  }
        ctx.MenuLines[1] = { Type = LINE_TYPE.MENU,  TextId = 0x232,  ValId = 0x106A  }
        ctx.MenuLines[2] = { Type = LINE_TYPE.MENU,  TextId = 0x233,  ValId = 0x106A  }
        ctx.MenuLines[3] = { Type = LINE_TYPE.MENU,  TextId = 0x234,  ValId = 0x106A  }
        ctx.MenuLines[4] = { Type = LINE_TYPE.MENU,  TextId = 0x235,  ValId = 0x106A  }
        ctx.MenuLines[6] = { Type = LINE_TYPE.MENU,  TextId = 0x90,  ValId = 0x1068  }

        ctx.SelLine = 6
        lastGoodMenu = menuId
    elseif (menuId==0x7CA5) then
        --M[Id=0x7CA5 P=0x0 N=0x1021 B=0x1021 Text="Gain Channel Select"[0xAD]]
        --L[#0 T=LM VId=0x1000 Text="Gain Channel"[0x89] val=7 N=(0->32,53,S=53) [53->85,53] MId=0x7CA5 ]

        ctx.Menu = { MenuId = 0x7CA5, TextId = 0xAD, PrevId = 0, NextId = 0x1054, BackId = 0x1054 }
        ctx.MenuLines[0] = { Type = LINE_TYPE.LIST_MENU, TextId = 0x89, ValId = 0x1000, Min=53, Max=85, Def=53, Val=7 }
        
        ctx.SelLine = 0
        lastGoodMenu = menuId
    elseif (menuId==0x7CA6) then
        --M[Id=0x7CA6 P=0x0 N=0x1021 B=0x1021 Text="FM Channel"[0x78]]
        --L[#0 T=LM VId=0x1000 Text="FM Channel"[0x78] val=7 N=(0->32,53,S=53) [53->85,53] MId=0x7CA6 ]

        ctx.Menu = { MenuId = 0x7CA6, TextId = 0x78, PrevId = 0, NextId = 0x1021, BackId = 0x1021 }
        ctx.MenuLines[0] = { Type = LINE_TYPE.LIST_MENU, TextId = 0x78, ValId = 0x1000, Min=53, Max=85, Def=53, Val=7 }
        
        ctx.SelLine = 0
        lastGoodMenu = menuId
    elseif (menuId==0x7CA7) then
        --M[Id=0x7CA7 P=0x0 N=0x1055 B=0x1055 Text="FM Channel"[0x78]]
        --L[#0 T=LM VId=0x1000 Text="FM Channel"[0x78] val=7 N=(0->32,53,S=53) [53->85,53] MId=0x7CA6 ]

        ctx.Menu = { MenuId = 0x7CA7, TextId = 0x78, PrevId = 0, NextId = 0x1055, BackId = 0x1055 }
        ctx.MenuLines[0] = { Type = LINE_TYPE.LIST_MENU, TextId = 0x78, ValId = 0x1000, Min=53, Max=85, Def=53, Val=0 }
        
        ctx.SelLine = 0
        lastGoodMenu = menuId
    elseif (menuId==0x0001) then 
        -- Save Settings and Reboot
        ctx.Menu = { MenuId = 0x0001, TextId = 0x009F, PrevId = 0, NextId = 0, BackId = 0x1000 }
        ctx.SelLine = menuLib.BACK_BUTTON

    else
        print("NOT IMPLEMENTED")
        ctx.Menu = { MenuId = 0x0002, Text = "NOT IMPLEMENTED", TextId = 0, PrevId = 0, NextId = 0, BackId = lastGoodMenu }
        ctx.SelLine = menuLib.BACK_BUTTON
    end

    menuLib.PostProcessMenu()
end

local function FC6250HX_loadMenu(menuId)
    menuLib.clearMenuLines()
    local ctx = menuLib.DSM_Context

    if (menuId==0x1000) then
        --M[Id=0x1000 P=0x0 N=0x0 B=0x0 Text="Main Menu"[0x4B]]
        --L[#0 T=M VId=0x1100 Text="Swashplate"[0xD3] MId=0x1000 ]
        --L[#1 T=M VId=0x1200 [0->0,2] Text="Tail rotor"[0xDD] MId=0x1000 ]

        --L[#2 T=M VId=0x1280 Text="Governor"[0xF2]

        --L[#3 T=M VId=0x1400 [0->0,2] Text="SAFE"[0xDA] MId=0x1000 ]
        --L[#5 T=M VId=0x1300 [0->0,2] Text="Setup"[0xDE] MId=0x1000 ]
        --L[#6 T=M VId=0x1700 [0->0,2] Text="System Setup"[0x86] MId=0x1000 ]

        ctx.Menu = { MenuId = 0x1000, TextId = 0x004B, PrevId = 0, NextId = 0, BackId = 0 }
        ctx.MenuLines[0] = { Type = LINE_TYPE.MENU, TextId = 0xD3, ValId = 0x1100 }
        ctx.MenuLines[1] = { Type = LINE_TYPE.MENU, TextId = 0xDD, ValId = 0x1200 }

        if (not RX_Initialized) then
          ctx.MenuLines[2] = { Type = LINE_TYPE.MENU, TextId = 0xF2, ValId = 0x1280 }
        end

        ctx.MenuLines[3] = { Type = LINE_TYPE.MENU, TextId = 0xDA, ValId = 0x1400 }

        ctx.MenuLines[5] = { Type = LINE_TYPE.MENU, TextId = 0xDE, ValId = 0x1300 }
        ctx.MenuLines[6] = { Type = LINE_TYPE.MENU, TextId = 0x86, ValId = 0x1700 }
        ctx.SelLine = 0
        lastGoodMenu = menuId
    elseif (menuId==0x1100) then
        --M[Id=0x1100 P=0x0 N=0x0 B=0x1000 Text="Swashplate"[0xD3]]
        --L[#0 T=M VId=0x1110 val=nil [0->0,2] Text="Roll"[0x40] MId=0x1100 ]
        --L[#1 T=M VId=0x1120 val=nil [0->0,2] Text="Pitch"[0x41] MId=0x1100 ]
        --L[#2 T=V_i8 VId=0x1103 Text="Agility"[0xD5] val=100 [0->200,100] MId=0x1100 ]

        ctx.Menu = { MenuId = 0x1100, TextId = 0xD3, PrevId = 0, NextId = 0, BackId = 0x1000 }
        ctx.MenuLines[0] = { Type = LINE_TYPE.MENU, TextId = 0x40, ValId = 0x1110, Min=0, Max=0, Def=3, Val=nil }
        ctx.MenuLines[1] = { Type = LINE_TYPE.MENU, TextId = 0x41, ValId = 0x1120, Min=0, Max=0, Def=2, Val=nil }
        ctx.MenuLines[2] = { Type = LINE_TYPE.VALUE_NUM_I8, TextId = 0xD5, ValId = 0x1103, Min=0, Max=200, Def=100, Val=100 }

        ctx.SelLine = 0
        lastGoodMenu = menuId
    elseif (menuId==0x1110) then
        --M[Id=0x1110 P=0x0 N=0x0 B=0x1100 Text="Roll"[0x40]]
        --L[#0 T=V_i16 VId=0x1111 Text="@ per sec"[0xDC] val=270 [0->900,270] MId=0x1110 ]
        --L[#1 T=V_nc VId=0x1112 Text="FLIGHT MODE"[0x8000] val=1 [0->5,0] MId=0x1110 ]
        --L[#2 T=V_i8 VId=0x1113  Text="Proportional"[0x71] val=100 [0->255,100] MId=0x1110 ]
        --L[#3 T=V_i8 VId=0x1114  Text="Integral"[0x72] val=100 [0->255,100] MId=0x1110 ]
        --L[#4 T=V_i8 VId=0x1115  Text="Derivate"[0x73] val=7 [0->255,7] MId=0x1110 ]

        ctx.Menu = { MenuId = 0x1110, TextId = 0x40, PrevId = 0, NextId = 0, BackId = 0x1100 }
        ctx.MenuLines[0] = { Type = LINE_TYPE.VALUE_NUM_I16, TextId = 0xDC, ValId = 0x1111, Min=0, Max=900, Def=270, Val=270 }
        ctx.MenuLines[1] = { Type = LINE_TYPE.VALUE_NUM_I8_NC, TextId = 0x8000, ValId = 0x1112, Min=0, Max=5, Def=0, Val=1 }
        ctx.MenuLines[2] = { Type = LINE_TYPE.VALUE_NUM_I8, TextId = 0x71, ValId = 0x1113, Min=0, Max=255, Def=100, Val=100 }
        ctx.MenuLines[3] = { Type = LINE_TYPE.VALUE_NUM_I8, TextId = 0x72, ValId = 0x1114, Min=0, Max=255, Def=100, Val=100 }
        ctx.MenuLines[4] = { Type = LINE_TYPE.VALUE_NUM_I8, TextId = 0x73, ValId = 0x1115, Min=0, Max=255, Def=7, Val=7 }

        ctx.SelLine = 0
        lastGoodMenu = menuId
    elseif (menuId==0x1120) then
        --M[Id=0x1120 P=0x0 N=0x0 B=0x1100 Text="Pitch"[0x41]]
        --L[#0 T=V_i16 VId=0x1121 Text="@ per sec"[0xDC] Val=270 [0->900,270] MId=0x1120 ]
        --L[#1 T=V_i8 VId=0x1212 Text="Start"[0x92] Val=nil [5->200,25] MId=0x1200 ]
        --L[#2 T=V_i8 VId=0x1213 Text="Stop"[0xD8] Val=nil [5->200,25] MId=0x1200 ]
        --L[#3 T=V_nc VId=0x1122 Text="                    FLIGHT MODE"[0x8000] Val=1 [0->5,0] MId=0x1120 ]
        --L[#4 T=V_i8 VId=0x1123 Text="Proportional"[0x71] Val=100 [0->255,100] MId=0x1120 ]
        --L[#5 T=V_i8 VId=0x1124 Text="Integral"[0x72] Val=100 [0->255,100] MId=0x1120 ]
        --L[#6 T=V_i8 VId=0x1125 Text="Derivate"[0x73] Val=14 [0->255,14] MId=0x1120 ]

        ctx.Menu = { MenuId = 0x1120, TextId = 0x41, PrevId = 0, NextId = 0, BackId = 0x1100 }
        ctx.MenuLines[0] = { Type = LINE_TYPE.VALUE_NUM_I16, TextId = 0xDC, ValId = 0x1121, Min=0, Max=900, Def=270, Val=270 }
        ctx.MenuLines[1] = { Type = LINE_TYPE.VALUE_NUM_I8, TextId = 0x92, ValId = 0x1123, Min=5, Max=200, Def=25, Val=25 }
        ctx.MenuLines[2] = { Type = LINE_TYPE.VALUE_NUM_I8, TextId = 0xD8, ValId = 0x1123, Min=5, Max=200, Def=26, Val=100 }

        ctx.MenuLines[3] = { Type = LINE_TYPE.VALUE_NUM_I8_NC, TextId = 0x8000, ValId = 0x1122, Min=0, Max=5, Def=0, Val=1 }

        ctx.MenuLines[4] = { Type = LINE_TYPE.VALUE_NUM_I8, TextId = 0x71, ValId = 0x1123, Min=0, Max=255, Def=100, Val=100 }
        ctx.MenuLines[5] = { Type = LINE_TYPE.VALUE_NUM_I8, TextId = 0x72, ValId = 0x1124, Min=0, Max=255, Def=95, Val=95 }
        ctx.MenuLines[6] = { Type = LINE_TYPE.VALUE_NUM_I8, TextId = 0x73, ValId = 0x1125, Min=0, Max=255, Def=45, Val=45 }

        ctx.SelLine = 0
        lastGoodMenu = menuId
    elseif (menuId==0x1200) then
        --M[Id=0x1200 P=0x0 N=0x0 B=0x1000 Text="Tail rotor"[0xDD]]
        --L[#0 T=V_i16 VId=0x1211 Text="@ per sec"[0xDC] Val=550 [0->1280,550] MId=0x1200 ]
        --L[#1 T=V_i8 VId=0x1212 Text="Start"[0x92] Val=25 [5->200,25] MId=0x1200 ]
        --L[#2 T=V_i8 VId=0x1213 Text="Stop"[0xD8] Val=25 [5->200,25] MId=0x1200 ]
        --L[#3 T=V_nc VId=0x1214 Text="                    FLIGHT MODE"[0x8000] Val=1 [0->5,0] MId=0x1200 ]
        --L[#4 T=V_i8 VId=0x1215 Text="Proportional"[0x71] Val=100 [0->255,100] MId=0x1200 ]
        --L[#5 T=V_i8 VId=0x1216 Text="Integral"[0x72] Val=95 [0->255,95] MId=0x1200 ]
        --L[#6 T=V_i8 VId=0x1217 Text="Derivate"[0x73] Val=45 [0->255,45] MId=0x1200 ]

        ctx.Menu = { MenuId = 0x1200, TextId = 0xDD, PrevId = 0, NextId = 0, BackId = 0x1000 }
        ctx.MenuLines[0] = { Type = LINE_TYPE.VALUE_NUM_I16, TextId = 0xDC, ValId = 0x1211, Min=0, Max=1280, Def=550, Val=550 }
        ctx.MenuLines[1] = { Type = LINE_TYPE.VALUE_NUM_I8, TextId = 0x92, ValId = 0x1212, Min=5, Max=200, Def=25, Val=25 }
        ctx.MenuLines[2] = { Type = LINE_TYPE.VALUE_NUM_I8, TextId = 0xD8, ValId = 0x1213, Min=5, Max=200, Def=26, Val=100 }

        ctx.MenuLines[3] = { Type = LINE_TYPE.VALUE_NUM_I8_NC, TextId = 0x8000, ValId = 0x1214, Min=0, Max=5, Def=0, Val=1 }

        ctx.MenuLines[4] = { Type = LINE_TYPE.VALUE_NUM_I8, TextId = 0x71, ValId = 0x1215, Min=0, Max=255, Def=100, Val=100 }
        ctx.MenuLines[5] = { Type = LINE_TYPE.VALUE_NUM_I8, TextId = 0x72, ValId = 0x1216, Min=0, Max=255, Def=95, Val=95 }
        ctx.MenuLines[6] = { Type = LINE_TYPE.VALUE_NUM_I8, TextId = 0x73, ValId = 0x1217, Min=0, Max=255, Def=45, Val=45 }

        ctx.SelLine = 0
        lastGoodMenu = menuId
    elseif (menuId==0x1280) then -- TODO
        --M[Id=0x1200 P=0x0 N=0x0 B=0x1000 Text="Governor"[0xF0]]
        --L[#4 T=V_i8 VId=0x1215 Text="Proportional"[0x71] Val=100 [0->255,100] MId=0x1200 ]
        --L[#5 T=V_i8 VId=0x1216 Text="Integral"[0x72] Val=95 [0->255,95] MId=0x1200 ]
        --L[#3 T=V_nc VId=0x1214 Text="                    FLIGHT MODE"[0x8000] Val=1 [0->5,0] MId=0x1200 ]
        --L[#6 T=V_i8 VId=0x1217 Text="Head Speed"[0x26B] Val=45 [0->255,45] MId=0x1200 ]

        ctx.Menu = { MenuId = 0x1280, TextId = 0xDD, PrevId = 0, NextId = 0, BackId = 0x1000 }
        ctx.MenuLines[0] = { Type = LINE_TYPE.VALUE_NUM_I8, TextId = 0x92, ValId = 0x1212, Min=5, Max=200, Def=25, Val=25 }
        ctx.MenuLines[1] = { Type = LINE_TYPE.VALUE_NUM_I8, TextId = 0xD8, ValId = 0x1213, Min=5, Max=200, Def=26, Val=100 }

        ctx.MenuLines[5] = { Type = LINE_TYPE.VALUE_NUM_I8_NC, TextId = 0x8000, ValId = 0x1214, Min=0, Max=5, Def=0, Val=1 }

        ctx.MenuLines[6] = { Type = LINE_TYPE.VALUE_NUM_I8, TextId = 0x71, ValId = 0x1215, Min=0, Max=255, Def=100, Val=100 }


        ctx.SelLine = 0
        lastGoodMenu = menuId
    elseif (menuId==0x1300) then
        --M[Id=0x1300 P=0x0 N=0x0 B=0x1000 Text="Setup"[0xDE]]
        --L[#0 T=M VId=0x1310 Text="Swashplate"[0xD3] MId=0x1300 ]
        --L[#1 T=M VId=0x1360 Text="Tail rotor"[0xDD] MId=0x1300 ]

        -- L[#2 T=M VId=0x13C0 Text="Throttle"[0x201]   MId=0x1300 A=0x0]
        -- L[#3 T=M VId=0x13B0 Text="Gyro settings"[0xF9]   MId=0x1300 A=0x0]

        --L[#4 T=LM_nc VId=0x1701 Text="FM Channel"[0x78] val=1 NL=(0->8,1,S=12) [12->20,13] MId=0x1300 ]
        --L[#5 T=LM_nc VId=0x1702 Text="Gain Channel"[0x89] val=0 NL=(0->8,1,S=12)] [12->20,13] MId=0x1300 ]
        --L[#6 T=LM_nc VId=0x1703 Text="Output Channel 6"[0x56] val=1 NL=(0->12,0,S=53) [53->65,53] MId=0x1300 ]
      
        ctx.Menu = { MenuId = 0x1300, TextId = 0xDE, PrevId = 0, NextId = 0, BackId = 0x1000 }
        ctx.MenuLines[0] = { Type = LINE_TYPE.MENU, TextId = 0xD3, ValId = 0x1310 }
        ctx.MenuLines[1] = { Type = LINE_TYPE.MENU, TextId = 0xDD, ValId = 0x1360 }

        if (not RX_Initialized) then
            ctx.MenuLines[2] = { Type = LINE_TYPE.MENU, TextId = 0x201, ValId = 0x13C0 }
            ctx.MenuLines[3] = { Type = LINE_TYPE.MENU, TextId = 0xF9, ValId = 0x13B0 }                
        end

        ctx.MenuLines[4] = { Type = LINE_TYPE.LIST_MENU_NC, TextId = 0x78, ValId = 0x1701, Min=12, Max=20, Def=13, Val=1 }
        ctx.MenuLines[5] = { Type = LINE_TYPE.LIST_MENU_NC, TextId = 0x89, ValId = 0x1702, Min=12, Max=20, Def=13, Val=0  }
        ctx.MenuLines[6] = { Type = LINE_TYPE.LIST_MENU_NC, TextId = 0x56, ValId = 0x1702, Min=53, Max=65, Def=53, Val=1 }

        ctx.SelLine = 0
        lastGoodMenu = menuId
    elseif (menuId==0x1310) then
        --M[Id=0x1310 P=0x0 N=0x0 B=0x1300 Text="Swashplate"[0xD3]]
        --L[#0 T=M VId=0x1330 Text="Output Setup"[0x49]   MId=0x1310 ]
        --L[#1 T=M VId=0x1320 Text="AFR"[0xDF]   MId=0x1310 ]
        --L[#2 T=V_% VId=0x1311 Text="E-Ring"[0xE4] Val=100 [50->150,100] MId=0x1310 ]
        --L[#3 T=V_% VId=0x1312 Text="Phasing"[0xE2] Val=0 [-45->45,0] MId=0x1310 ]
        --L[#4 T=V_% VId=0x1313 Text="Decay"[0x208] Val=50 [0->100,50] MId=0x1310 ]

        ctx.Menu = { MenuId = 0x1310, TextId = 0xD3, PrevId = 0, NextId = 0, BackId = 0x1300 }
        ctx.MenuLines[0] = { Type = LINE_TYPE.MENU, TextId = 0x49, ValId = 0x1330 }
        ctx.MenuLines[1] = { Type = LINE_TYPE.MENU, TextId = 0xDF, ValId = 0x1320 }
        ctx.MenuLines[4] = { Type = LINE_TYPE.VALUE_PERCENT, TextId = 0xE4, ValId = 0x1311, Min=50, Max=150, Def=100, Val=100 }
        ctx.MenuLines[5] = { Type = LINE_TYPE.VALUE_PERCENT, TextId = 0xE2, ValId = 0x1312, Min=-45, Max=45, Def=0, Val=0  }
        ctx.MenuLines[6] = { Type = LINE_TYPE.VALUE_PERCENT, TextId = 0x208, ValId = 0x1313, Min=0, Max=100, Def=50, Val=50 }

        ctx.SelLine = 0
        lastGoodMenu = menuId
    elseif (menuId==0x1320) then
        --M[Id=0x1320 P=0x0 N=0x0 B=0x1310 Text="AFR"[0xDF]]
        --L[#0 T=V_% VId=0x1321 Text="Roll"[0x40] Val=-75 % [-127->127,-75] MId=0x1320 ]
        --L[#1 T=V_% VId=0x1322 Text="Pitch"[0x41] Val=-75 % [-127->127,-75] MId=0x1320 ]
        --L[#2 T=V_% VId=0x1323 Text="Collective"[0xE0] Val=45 % [5->127,45] MId=0x1320 ]
        --L[#3 T=V_% VId=0x1324 Text="Differential"[0x45] Val=0 % [-25->25,0] MId=0x1320 ]

        ctx.Menu = { MenuId = 0x1320, TextId = 0xDF, PrevId = 0, NextId = 0, BackId = 0x1310 }
        ctx.MenuLines[0] = { Type = LINE_TYPE.VALUE_PERCENT, TextId = 0x40, ValId = 0x1321, Min=-127, Max=127, Def=-75, Val=-75 }
        ctx.MenuLines[1] = { Type = LINE_TYPE.VALUE_PERCENT, TextId = 0x41, ValId = 0x1322, Min=-127, Max=127, Def=-75, Val=-75  }
        ctx.MenuLines[2] = { Type = LINE_TYPE.VALUE_PERCENT, TextId = 0xE0, ValId = 0x1323, Min=5, Max=127, Def=45, Val=45  }
        ctx.MenuLines[3] = { Type = LINE_TYPE.VALUE_PERCENT, TextId = 0x45, ValId = 0x1324, Min=-25, Max=25, Def=0, Val=0 }

        ctx.SelLine = 0
        lastGoodMenu = menuId
    elseif (menuId==0x1330) then
        --M[Id=0x1330 P=0x0 N=0x0 B=0x1310 Text="Output Setup"[0x49]]

        ---- Full version 
        --L[#0 T=LM_nc2 VId=0x1331 Text="Frame Rate"[0x85] Val=nil NL=(0->5,0,S=136) [136->141,136] MId=0x1330 A=0x0]
        --L[#1 T=M VId=0x1334 Text="Swash Type"[0xE5]   MId=0x1330 A=0x0]
        --L[#2 T=M VId=0x1332 Text="Direction"[0xF6]   MId=0x1330 A=0x0]

        --L[#3 T=M VId=0x1331 Text="Subtrim"[0xE1]   MId=0x1330 ]

        ctx.Menu = { MenuId = 0x1330, TextId = 0x49, PrevId = 0, NextId = 0, BackId = 0x1310 }

        if (not RX_Initialized) then
            ctx.MenuLines[0] = { Type = LINE_TYPE.LIST_MENU_NC, TextId = 0x85, ValId = 0x1331, Min=136, Max=141, Def=136, Val=0 }
            ctx.MenuLines[1] = { Type = LINE_TYPE.MENU, TextId = 0xE5, ValId = 0x1334 }
            ctx.MenuLines[2] = { Type = LINE_TYPE.MENU, TextId = 0xF6, ValId = 0x1332 }
            ctx.MenuLines[3] = { Type = LINE_TYPE.MENU, TextId = 0xE1, ValId = 0x1331 }
            ctx.SelLine = 0
        else
            ctx.MenuLines[0] = { Type = LINE_TYPE.MENU, TextId = 0xE1, ValId = 0x1331 }
            ctx.SelLine = 0
        end

        lastGoodMenu = menuId
    elseif (menuId==0x1331) then
        --M[Id=0x1331 P=0x0 N=0x0 B=0x1330 Text="Subtrim"[0xE1]]
        --L[#0 T=V_s16 VId=0x1332 Text="Output Channel 1"[0x51] Val=57 [-82->82,0] MId=0x1331 ]
        --L[#1 T=V_s16 VId=0x1333 Text="Output Channel 2"[0x52] Val=-17 [-82->82,0] MId=0x1331 ]
        --L[#2 T=V_s16 VId=0x1334 Text="Output Channel 3"[0x53] Val=-53 [-82->82,0] MId=0x1331 ]

        ctx.Menu = { MenuId = 0x1331, TextId = 0xE1, PrevId = 0, NextId = 0, BackId = 0x1330 }
        ctx.MenuLines[0] = { Type = LINE_TYPE.VALUE_NUM_SI16, TextId = 0x51, ValId = 0x1332, Min=-82, Max=82, Def=0, Val=57 }
        ctx.MenuLines[1] = { Type = LINE_TYPE.VALUE_NUM_SI16, TextId = 0x52, ValId = 0x1333, Min=-82, Max=82, Def=0, Val=-17 }
        ctx.MenuLines[2] = { Type = LINE_TYPE.VALUE_NUM_SI16, TextId = 0x53, ValId = 0x1334, Min=-82, Max=82, Def=0, Val=-53 }
     
        ctx.SelLine = 0
        lastGoodMenu = menuId
    elseif (menuId==0x1332) then
        --M[Id=0x1332 P=0x0 N=0x0 B=0x1330 Text="Direction"[0xF6]]
        --L[#0 T=LM_tog VId=0x1333 Text="Output Channel 1"[0x51] Val=nil NL=(0->1,0,S=142) [142->143,142] MId=0x1332 A=0x0]
        --L[#1 T=LM_tog VId=0x1334 Text="Output Channel 2"[0x52] Val=nil NL=(0->1,0,S=142) [142->143,142] MId=0x1332 A=0x0]
        --L[#2 T=LM_tog VId=0x1335 Text="Output Channel 3"[0x53] Val=nil NL=(0->1,0,S=142) [142->143,142] MId=0x1332 A=0x0]

        ctx.Menu = { MenuId = 0x1332, TextId = 0xF6, PrevId = 0, NextId = 0, BackId = 0x1330 }
        ctx.MenuLines[0] = { Type = LINE_TYPE.LIST_MENU_TOG, TextId = 0x51, ValId = 0x1333, Min=142, Max=143, Def=142, Val=0 }
        ctx.MenuLines[1] = { Type = LINE_TYPE.LIST_MENU_TOG, TextId = 0x52, ValId = 0x1334, Min=142, Max=143, Def=142, Val=0 }
        ctx.MenuLines[2] = { Type = LINE_TYPE.LIST_MENU_TOG, TextId = 0x53, ValId = 0x1335, Min=142, Max=143, Def=142, Val=0 }
     
        ctx.SelLine = 0
        lastGoodMenu = menuId
    elseif (menuId==0x1334) then
        -- M[Id=0x1334 P=0x0 N=0x0 B=0x1330 Text="Swashplate"[0xD3]]
        -- L[#6 T=LM_tog VId=0x1335 Text="Swash Type"[0xE5] Val=nil NL=(0->8,0,S=144) [144->152,144] MId=0x1334 A=0x0]

        ctx.Menu = { MenuId = 0x1334, TextId = 0xD3, PrevId = 0, NextId = 0, BackId = 0x1330 }
        ctx.MenuLines[6] = { Type = LINE_TYPE.LIST_MENU_NC, TextId = 0xE5, ValId = 0x1335, Min=144, Max=152, Def=144, Val=0 }
     
        ctx.SelLine = 6
        lastGoodMenu = menuId
    elseif (menuId==0x1360) then
        --M[Id=0x1360 P=0x0 N=0x0 B=0x1300 Text="Tail rotor"[0xDD]]
        --L[#0 T=M VId=0x1390 Text="Advanced Setup"[0x99]   MId=0x1360 ]

        ctx.Menu = { MenuId = 0x1360, TextId = 0xDD, PrevId = 0, NextId = 0, BackId = 0x1000 }
        ctx.MenuLines[0] = { Type = LINE_TYPE.MENU, TextId = 0x99, ValId = 0x1390 }

        ctx.SelLine = 0
        lastGoodMenu = menuId
    elseif (menuId==0x1390) then
        --M[Id=0x1390 P=0x0 N=0x0 B=0x1360 Text="Phasing"[0xE2]]
        --L[#0 T=V_% VId=0x1311 Text="Left"[0xE7] Val=0 % [-45->45,0] MId=0x1390 ]
        --L[#1 T=V_% VId=0x1312 Text="Right"[0xE8] Val=0 % [-45->45,0] MId=0x1390 ]

        ctx.Menu = { MenuId = 0x1390, TextId = 0xE2, PrevId = 0, NextId = 0, BackId = 0x1360 }
        ctx.MenuLines[0] = { Type = LINE_TYPE.VALUE_PERCENT, TextId = 0xE2, ValId = 0x1311, Min=-45, Max=45, Def=0, Val=0 }
        ctx.MenuLines[1] = { Type = LINE_TYPE.VALUE_PERCENT, TextId = 0xE8, ValId = 0x1312,Min=-45, Max=45, Def=0, Val=0  }


        ctx.SelLine = 0
        lastGoodMenu = menuId

    elseif (menuId==0x13B0) then
        -- M[Id=0x13B0 P=0x0 N=0x0 B=0x1300 Text="Gyro settings"[0xF9]]
        -- L[#0 T=M VId=0x13B1 Text="Orientation"[0x80]   MId=0x13B0 A=0x0]

        ctx.Menu = { MenuId = 0x13B0, TextId = 0xF9, PrevId = 0, NextId = 0, BackId = 0x1300 }
        ctx.MenuLines[0] = { Type = LINE_TYPE.MENU, TextId = 0x80, ValId = 0x13B1 }

        ctx.SelLine = 0
        lastGoodMenu = menuId
    elseif (menuId==0x13B1) then
        -- M[Id=0x13B1 P=0x0 N=0x0 B=0x13B5 Text="Gyro settings"[0xF9]]
        --L[#6 T=LM_ori VId=0x13B2 Text="Orientation"[0x80] Val=nil NL=(0->7,0,S=203) [203->210,203] MId=0x13B1 A=0x0]

        ctx.Menu = { MenuId = 0x13B1, TextId = 0xF9, PrevId = 0, NextId = 0, BackId = 0x13B5 }
        ctx.MenuLines[6] = { Type = LINE_TYPE.LIST_MENU_ORI, TextId = 0x80, ValId = 0x13B2, Min=203, Max=210, Def=203, Val=0 }
     
        ctx.SelLine = 6
        lastGoodMenu = menuId
    elseif (menuId==0x13B5) then
        --M[Id=0x13B5 P=0x0 N=0x0 B=0x13B0 Text="Calibrate Sensor"[0xC7]]
        --L[#3 T=M VId=0x13B6 Text="Begin"[0x91]   MId=0x13B5 A=0x0]

        ctx.Menu = { MenuId = 0x13B5, TextId = 0xC7, PrevId = 0, NextId = 0, BackId = 0x13B0 }
        ctx.MenuLines[3] = { Type = LINE_TYPE.MENU, TextId = 0x91, ValId = 0x13B6 }

        ctx.SelLine = 3
        lastGoodMenu = menuId
    elseif (menuId==0x13B6) then
        --M[Id=0x13B6 P=0x0 N=0x0 B=0x13B0 Text="Calibrate Sensor"[0xC7]]
        --L[#3 T=M VId=0x13B0 Text="Sensor is Calibrating.. Wait"[0xC8]   MId=0x13B6 A=0x0]

        ctx.Menu = { MenuId = 0x13B6, TextId = 0xC7, PrevId = 0, NextId = 0, BackId = 0x13B0 }
        ctx.MenuLines[3] = { Type = LINE_TYPE.MENU, TextId = 0xC8, ValId = 0x17F1 }

        ctx.SelLine = 3
        lastGoodMenu = menuId
    elseif (menuId==0x13C0) then
        --M[Id=0x13C0 P=0x0 N=0x0 B=0x1300 Text="Throttle"[0x201]]
        --L[#0 T=M VId=0x13C1 Text="Failsafe"[0x4A]   MId=0x13C0 A=0x0]
        --L[#1 T=V_% VId=0x13C2 Text="Hover"[0x204] Val=nil [0->100,65] MId=0x13C0 A=0x10]
        --L[#2 T=M VId=0x13D0 Text="Governor"[0xF2]   MId=0x13C0 A=0x0]
        --L[#4 T=V_nc VId=0x13C3 Text="Flight Mode"[0x8000] Val=nil [0->5,0] MId=0x13C0 A=0x5]
        --L[#5 T=V_% VId=0x13C4 Text="Offset"[0x1AA] Val=nil [-25->25,0] MId=0x13C0 A=0x10]
        --L[#6 T=V_i8 VId=0x13C5 Text="Soft Start"[0xF4] Val=nil [0->250,0] MId=0x13C0 A=0x0]

        ctx.Menu = { MenuId = 0x13C0, TextId = 0x201, PrevId = 0, NextId = 0, BackId = 0x1300 }
        ctx.MenuLines[0] = { Type = LINE_TYPE.MENU, TextId = 0x4A, ValId = 0x13C1 }
        ctx.MenuLines[1] = { Type = LINE_TYPE.VALUE_PERCENT, TextId = 0x204, ValId = 0x13C2, Min=0, Max=100, Def=65, Val=65  }
        ctx.MenuLines[2] = { Type = LINE_TYPE.MENU, TextId = 0xF2, ValId = 0x13D0 }

        ctx.MenuLines[4] = { Type = LINE_TYPE.VALUE_NUM_I8_NC, TextId = 0x8000, ValId = 0x13C3, Min=0, Max=5, Def=0, Val=1 }
        ctx.MenuLines[5] = { Type = LINE_TYPE.VALUE_PERCENT, TextId = 0x1AA, ValId = 0x13C4, Min=-25, Max=25, Def=0, Val=0  }
        ctx.MenuLines[6] = { Type = LINE_TYPE.VALUE_NUM_I8, TextId = 0xF4, ValId = 0x13C5, Min=0, Max=250, Def=0, Val=1 }

        ctx.SelLine = 0
        lastGoodMenu = menuId
    elseif (menuId==0x13C1) then
        --M[Id=0x13C1 P=0x0 N=0x0 B=0x13C0 Text="Failsafe"[0x4A]]
        --L[#2 T=M VId=0x13C3 Text="Capture Failsafe Positions"[0x9A]   MId=0x13C1 A=0x0]

        ctx.Menu = { MenuId = 0x13C1, TextId = 0x4A, PrevId = 0, NextId = 0, BackId = 0x13C0 }
        ctx.MenuLines[2] = { Type = LINE_TYPE.MENU, TextId = 0x9A, ValId = 0x13C3 }

        ctx.SelLine = 2
        lastGoodMenu = menuId
    elseif (menuId==0x13D0) then
        --M[Id=0x13D0 P=0x0 N=0x0 B=0x13C0 Text="Governor"[0xF2]]
        --L[#0 T=LM_ori VId=0x13D1 Text="Governor"[0xF2] Val=nil NL=(0->1,0,S=244) [244->245,244] MId=0x13D0 A=0x0]
        --L[#1 T=V_i8 VId=0x13D2 Text="Main Gear"[0x26D] Val=nil [1->255,170] MId=0x13D0 A=0x0]
        --L[#2 T=V_i8 VId=0x13D3 Text="Pinion"[0x26C] Val=nil [1->255,20] MId=0x13D0 A=0x0]
        --L[#3 T=V_i8 VId=0x13D5 Text="Low Throttle"[0xEA] Val=nil [1->100,75] MId=0x13D0 A=0x0]
        --L[#4 T=V_i8 VId=0x13D6 Text="Filter"[0x1F1] Val=nil [1->65,35] MId=0x13D0 A=0x0]
        --L[#5 T=M VId=0x13E0 Text="RPM Sensor"[0x26F]   MId=0x13D0 A=0x0]


        ctx.Menu = { MenuId = 0x13D0, TextId = 0xF2, PrevId = 0, NextId = 0, BackId = 0x13C0 }
        ctx.MenuLines[0] = { Type = LINE_TYPE.LIST_MENU_ORI, TextId = 0x0F2, ValId = 0x13D1, Min=244, Max=245, Def=244, Val=0  }
        ctx.MenuLines[1] = { Type = LINE_TYPE.VALUE_NUM_I8,  TextId = 0x26D, ValId = 0x13D2, Min=1, Max=255, Def=170, Val=170 }
        ctx.MenuLines[2] = { Type = LINE_TYPE.VALUE_NUM_I8,  TextId = 0x26C, ValId = 0x13D3, Min=1, Max=255, Def=20, Val=20 }
        ctx.MenuLines[3] = { Type = LINE_TYPE.VALUE_NUM_I8,  TextId = 0x0EA, ValId = 0x13D5, Min=1, Max=100, Def=75, Val=75 }
        ctx.MenuLines[4] = { Type = LINE_TYPE.VALUE_NUM_I8,  TextId = 0x1F1, ValId = 0x13D6, Min=1, Max=65, Def=35, Val=35 }
        ctx.MenuLines[5] = { Type = LINE_TYPE.MENU,          TextId = 0x26F, ValId = 0x13E0 }
        
        ctx.SelLine = 0
        lastGoodMenu = menuId
    elseif (menuId==0x13E0) then
        --M[Id=0x13E0 P=0x0 N=0x0 B=0x13D0 Text="Governor"[0xF2]]
        --L[#0 T=LM_ori VId=0x13D1 Text="RPM Sensor"[0x26F] Val=nil NL=(0->1,0,S=244) [244->245,244] MId=0x13D0 A=0x0]

        ctx.Menu = { MenuId = 0x13E0, TextId = 0xF2, PrevId = 0, NextId = 0, BackId = 0x13D0 }
        ctx.MenuLines[3] = { Type = LINE_TYPE.LIST_MENU_TOG, TextId = 0x26F, ValId = 0x13E3, Min=142, Max=143, Def=142, Val=0  }
        
        ctx.SelLine = 3
        lastGoodMenu = menuId
    elseif (menuId==0x1400) then
        --M[Id=0x1400 P=0x0 N=0x0 B=0x1000 Text="SAFE"[0xDA]]
        --L[#0 T=M VId=0x1410 Text="Stability"[0xDB] MId=0x1400 ]
        --L[#1 T=M VId=0x140 Text="Panic"[0x8B] MId=0x1400 ]
        --L[#2 T=M VId=0x1420 Text="Attitude Trim"[0x1E6] MId=0x1400 ]

        ctx.Menu = { MenuId = 0x1400, TextId = 0xDA, PrevId = 0, NextId = 0, BackId = 0x1000 }
        ctx.MenuLines[0] = { Type = LINE_TYPE.MENU, TextId = 0xDB, ValId = 0x1410 }
        ctx.MenuLines[1] = { Type = LINE_TYPE.MENU, TextId = 0x8B, ValId = 0x140 }
        ctx.MenuLines[2] = { Type = LINE_TYPE.MENU, TextId = 0x1E6, ValId = 0x1420 }

        ctx.SelLine = 0
        lastGoodMenu = menuId
    elseif (menuId==0x1410) then
       -- M[Id=0x1410 P=0x0 N=0x0 B=0x1400 Text="Stability"[0xDB]]
        --L[#0 T=V_i8 VId=0x1411 Text="Gain"[0x43] val=50 [5->200,50] MId=0x1410 ]
        --L[#1 T=V_i8 VId=0x1412 Text="Envelope"[0x1E7] val=45 [5->90,45] MId=0x1410 ]
        --L[#3 T=V_nc VId=0x1413 Text="FLIGHT MODE"[0x8000] val=1 [0->5,0] MId=0x1410 ]
        --L[#4 T=LM_tog VId=0x1414 Text="Stability"[0xDB] val=1 NL=(0->1,1,S=1) [1->2,1]  MId=0x1410 ]
    
        ctx.Menu = { MenuId = 0x1410, TextId = 0xDB, PrevId = 0, NextId = 0, BackId = 0x1400 }
        ctx.MenuLines[0] = { Type = LINE_TYPE.VALUE_NUM_I8, TextId = 0x43, ValId = 0x1411,    Min=0, Max=200, Def=50, Val=50 }
        ctx.MenuLines[1] = { Type = LINE_TYPE.VALUE_NUM_I8, TextId = 0x1E7, ValId = 0x1412,   Min=0, Max=90, Def=45, Val=45 }
        ctx.MenuLines[3] = { Type = LINE_TYPE.VALUE_NUM_I8_NC, TextId = 0x8000, ValId = 0x1413, Min=0, Max=5, Def=0, Val=1 }
        ctx.MenuLines[4] = { Type = LINE_TYPE.LIST_MENU_TOG, TextId = 0xDB, ValId = 0x1414,      Min=1, Max=2, Def=1, Val=1 }

        ctx.SelLine = 0
        lastGoodMenu = menuId

    elseif (menuId==0x140) then
        --M[Id=0x140 P=0x0 N=0x0 B=0x1400 Text="Panic"[0x8B]]
        --L[#0 T=V_i8 VId=0x141 Text="Envelope"[0x1E7] val=30 [5->90,45] MId=0x140 ]
        --L[#1 T=V_i8 VId=0x142 Text="Yaw"[0x42] val=30 [25->100,50] MId=0x140 ]
     
         ctx.Menu = { MenuId = 0x140, TextId = 0x8B, PrevId = 0, NextId = 0, BackId = 0x1400 }
         ctx.MenuLines[0] = { Type = LINE_TYPE.VALUE_NUM_I8, TextId = 0x1E7, ValId = 0x141, Min=5, Max=90, Def=45, Val=30 }
         ctx.MenuLines[1] = { Type = LINE_TYPE.VALUE_NUM_I8, TextId = 0x42, ValId = 0x142, Min=25, Max=100, Def=50, Val=30 }
         
         ctx.SelLine = 0
         lastGoodMenu = menuId
    elseif (menuId==0x1420) then
        --M[Id=0x1420 P=0x0 N=0x0 B=0x1400 Text="Attitude Trim"[0x1E6]]
        --L[#0 T=V_s16 VId=0x1421 Text="Roll"[0x40] val=274 [-850->850,450] MId=0x1420 ]
        --L[#1 T=V_s16 VId=0x1422 Text="Pitch"[0x41] val=58 [-850->850,0] MId=0x1420 ]

        ctx.Menu = { MenuId = 0x1420, TextId = 0x1E6, PrevId = 0, NextId = 0, BackId = 0x1400 }
        ctx.MenuLines[0] = {  Type = LINE_TYPE.VALUE_NUM_SI16, TextId = 0x40, ValId = 0x1421, Min=-850, Max=850, Def=450, Val=274 }
        ctx.MenuLines[1] = {  Type = LINE_TYPE.VALUE_NUM_SI16, TextId = 0x41, ValId = 0x1422, Min=-850, Max=850, Def=0, Val=58 }
        
        ctx.SelLine = 0
        lastGoodMenu = menuId
    elseif (menuId==0x1700) then
        --M[Id=0x1700 P=0x0 N=0x0 B=0x1000 Text="System Setup"[0x86]]
        --L[#0 T=M VId=0x17F0 Text="Calibrate Sensor"[0xC7]   MId=0x1700 ]
        --L[#1 T=M VId=0x17E0 Text="Factory Reset"[0x97]   MId=0x1700 ]

        ctx.Menu = { MenuId = 0x1700, TextId = 0x86, PrevId = 0, NextId = 0, BackId = 0x1000 }
        ctx.MenuLines[0] = { Type = LINE_TYPE.MENU, TextId = 0xC7, ValId = 0x17F0 }
        ctx.MenuLines[1] = { Type = LINE_TYPE.MENU, TextId = 0x97, ValId = 0x17E0 }

        ctx.SelLine = 0
        lastGoodMenu = menuId
    elseif (menuId==0x17E0) then
        --M[Id=0x17E0 P=0x0 N=0x0 B=0x1700 Text="Factory Reset"[0x98]]
        --[#3 T=M VId=0x17E1 Text="Apply"[0x90]   MId=0x17E0 ]

        ctx.Menu = { MenuId = 0x17E0, TextId = 0x98, PrevId = 0, NextId = 0, BackId = 0x1700 }
        ctx.MenuLines[3] = { Type = LINE_TYPE.MENU, TextId = 0x90, ValId = 0x17E1 }

        ctx.SelLine = 3
        lastGoodMenu = menuId
    elseif (menuId==0x17F0) then
        --M[Id=0x17F0 P=0x0 N=0x0 B=0x1700 Text="Calibrate Sensor"[0xC7]]
        --L[#3 T=M VId=0x17F1 Text="Begin"[0x91]   MId=0x17F0 ]

        ctx.Menu = { MenuId = 0x17F0, TextId = 0xC7, PrevId = 0, NextId = 0, BackId = 0x1700 }
        ctx.MenuLines[3] = { Type = LINE_TYPE.MENU, TextId = 0x91, ValId = 0x17F1 }

        ctx.SelLine = 3
        lastGoodMenu = menuId
    elseif (menuId==0x17F1) then
        --M[Id=0x17F1 P=0x0 N=0x0 B=0x1700 Text="Calibrate Sensor"[0xC7]]
        --L[#3 T=M VId=0x17F1 Text="Complete"[0x93]   MId=0x17F0 ]

        ctx.Menu = { MenuId = 0x17F1, TextId = 0xC7, PrevId = 0, NextId = 0, BackId = 0x1700 }
        ctx.MenuLines[3] = { Type = LINE_TYPE.MENU, TextId = 0x93, ValId = 0x1700 }

        ctx.SelLine = 3
        lastGoodMenu = menuId
    elseif (menuId==0x0001) then 
        -- Save Settings and Reboot
        ctx.Menu = { MenuId = 0x0001, TextId = 0x009F, PrevId = 0, NextId = 0, BackId = 0x1000 }
        ctx.SelLine = menuLib.BACK_BUTTON
    else
        ctx.Menu = { MenuId = 0x0002, Text = "NOT IMPLEMENTED", TextId = 0, PrevId = 0, NextId = 0, BackId = lastGoodMenu }
        ctx.SelLine = menuLib.BACK_BUTTON
    end

    menuLib.PostProcessMenu()
end


local function loadMenu(menuId)
    menuLib.clearMenuLines()
    local ctx = menuLib.DSM_Context

    if (menuId==0x1000) then
        --M[Id=0x1000 P=0x0 N=0x0 B=0x0 Text="Main Menu"]
        --L[#0 T=M VId=0x1010 val=nil [0->0,3] Text="Gyro settings" MId=0x1000 ]
        --L[#1 T=M VId=0x105E val=nil [0->0,2] Text="Other settings" MId=0x1000 ]

        ctx.Menu = { MenuId = 0x1000, Text = "RX SIMULATION", PrevId = 0, NextId = 0, BackId = 0, TextId=0 }
        ctx.MenuLines[0] = { MenuId = 0x1000, Type = LINE_TYPE.MENU, Text = "AR630/631/637 (NEW)", ValId = 0x1001,TextId=0 }
        ctx.MenuLines[1] = { MenuId = 0x1000, Type = LINE_TYPE.MENU, Text = "AR630/631/637 (INITIALIZED)", ValId = 0x1002,  TextId=0 }
        ctx.MenuLines[4] = { MenuId = 0x1000, Type = LINE_TYPE.MENU, Text = "FC6250HX", ValId = 0x1005, TextId=0 }
        ctx.MenuLines[5] = { MenuId = 0x1000, Type = LINE_TYPE.MENU, Text = "FC6250HX (UNLOCKED)", ValId = 0x1006, TextId=0 }

        ctx.SelLine = 0
        lastGoodMenu = menuId
    elseif (menuId==0x1001) then
        RX_Initialized =  false
        ctx.RX.Id   =  menuLib.RX.AR631
        ctx.RX.Name = "AR630/631/637-SIM"
        ctx.RX.Version = "2.38.5"      
        RX_loadMenu = AR631_loadMenu
        RX_loadMenu(0x01000)
    elseif (menuId==0x1002) then
        RX_Initialized =  true
        ctx.RX.Id   =  menuLib.RX.AR631
        ctx.RX.Name = "AR630/631/637-SIM"
        ctx.RX.Version = "2.38.5"
        RX_loadMenu = AR631_loadMenu
        RX_loadMenu(0x01000)
    elseif (menuId==0x1005) then
        RX_Initialized =  true
        ctx.RX.Id   =  menuLib.RX.FC6250HX
        ctx.RX.Name = "FC6250HX-SIM"
        ctx.RX.Version = "5.6.255"

        RX_loadMenu = FC6250HX_loadMenu
        RX_loadMenu(0x01000)
    elseif (menuId==0x1006) then
        RX_Initialized =  false
        ctx.RX.Id   =  menuLib.RX.FC6250HX
        ctx.RX.Name = "FC6250HX-SIM"
        ctx.RX.Version = "5.6.52"

        RX_loadMenu = FC6250HX_loadMenu
        RX_loadMenu(0x01000)
    end
end




local function SIM_Send_Receive()
    local ctx = menuLib.DSM_Context
    --if (DEBUG_ON) then Log.LOG_write("%3.3f %s: ", menuLib.getElapsedTime(), menuLib.phase2String(ctx.Phase)) end

    if ctx.Phase == PHASE.RX_VERSION then -- request RX version
        ctx.RX.Name = "SIMULATOR"
        ctx.RX.Version = SIM_LIB_VERSION
        ctx.Phase = PHASE.MENU_TITLE
        ctx.Menu.MenuId=0

        ctx.Refresh_Display = true
        RX_loadMenu = loadMenu
        
    elseif ctx.Phase == PHASE.WAIT_CMD then 
        
    elseif ctx.Phase == PHASE.MENU_TITLE then -- request menu title
        if ctx.Menu.MenuId == 0 then  -- First time loading a menu ?
            RX_loadMenu(0x01000)
        else
            RX_loadMenu(ctx.Menu.MenuId)
        end
        ctx.Phase = PHASE.WAIT_CMD
        ctx.Refresh_Display = true

    elseif ctx.Phase == PHASE.VALUE_CHANGING then -- send value
        local line = ctx.MenuLines[ctx.SelLine] -- Updated Value of SELECTED line
        if (DEBUG_ON) then Log.LOG_write("%3.3f %s: ", menuLib.getElapsedTime(), menuLib.phase2String(ctx.Phase)) end
        if (DEBUG_ON) then Log.LOG_write("SEND SIM_updateMenuValue(ValueId=0x%X Text=\"%s\" Value=%s)\n", line.ValId, line.Text, menuLib.lineValue2String(line)) end
        ctx.Phase = PHASE.VALUE_CHANGING_WAIT

    elseif ctx.Phase == PHASE.VALUE_CHANGING_WAIT then
        local line = ctx.MenuLines[ctx.SelLine]

    elseif ctx.Phase == PHASE.VALUE_CHANGE_END then -- send value
        local line = ctx.MenuLines[ctx.SelLine] -- Updated Value of SELECTED line
        if (DEBUG_ON) then Log.LOG_write("%3.3f %s: ", menuLib.getElapsedTime(), menuLib.phase2String(ctx.Phase)) end
        if (DEBUG_ON) then Log.LOG_write("SEND SIM_updateMenuValue(ValueId=0x%X Text=\"%s\" Value=%s)\n", line.ValId, line.Text, menuLib.lineValue2String(line)) end
        if (DEBUG_ON) then Log.LOG_write("SEND SIM_validateMenuValue(ValueId=0x%X Text=\"%s\" Value=%s)\n", line.ValId, line.Text, menuLib.lineValue2String(line)) end
        ctx.Phase = PHASE.WAIT_CMD
    
    elseif ctx.Phase == PHASE.EXIT then
       ctx.Phase=PHASE.EXIT_DONE
    end
end

local FileState = {}

-- Initial Setup
local function Sim_Init()
    local ctx = menuLib.DSM_Context
    ctx.Phase = PHASE.INIT

    local ver, radio, maj, minor, rev, osname = getVersion()
    if (osname==nil) then osname = "OpenTX" end -- OTX 2.3.14 and below returns nil
    IS_EDGETX = string.sub(osname,1,1) == 'E'
end

local function SIM_Done()
    local ctx = menuLib.DSM_Context
    ctx.Phase = PHASE.EXIT_DONE
end


local function SIM_Run()
    if (menuLib.DSM_Context.Phase == PHASE.INIT) then 
        if (IS_EDGETX) then
            menuLib.LoadTextFromFile(MSG_FILE,13)
            menuLib.DSM_Context.Phase = PHASE.RX_VERSION
        else  -- Incremental initialization
            lcd.clear()       
            lcd.drawText(30, 50, "Loading Msg file: "..(FileState.lineNo or 0))
            if (menuLib.INC_LoadTextFromFile(MSG_FILE, FileState)==1) then
                menuLib.DSM_Context.Phase = PHASE.RX_VERSION
            end
            return          
        end
    end
    
    return SIM_Send_Receive()
end

return { init=Sim_Init, run=SIM_Run, done=SIM_Done }
