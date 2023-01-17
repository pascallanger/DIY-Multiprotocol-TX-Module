# Forward Programing Protocol

## Introduction
DSM, DSMX and DSM Forward Programming are propietary protocol from the **Spektrum** radio brand. Since they don't make this information public, we have to reverse engineer it by analyzing the data exchanged between the RX and TX.

This document descrives what we know so far.

Thanks to **Pascal Langer** (Author of the Multi-Module) for the initial reverse engineering of the protocol and first version of the code that has been used for a while (Version 0.2)

Thanks to **Francisco Arzu** for taking the time to continue the work on reverse engineering, documenting and making the code more understandable.

New Capabilities in Version 0.5
- Log files of the conversation between RX/TX 
- Improve the GUI  (EdgeTX touch screen)
- Reversed engineer other things to make it work completly.

# Menu Title and Lines

The menu to be displayed is stored at the RX, the GUI only renders the menu title and menu lines received. The tipical conversation with the RX will be to ask for a menu (using the menuId number), and then wait for the data to come. The first thing will be the Menu (header) data, later we request the next 6 lines (one at a time), and optionally the values for each line.

A typical exchange will look like this in the log:

    SEND DSM_getMenu(MenuId=0x1010 LastSelectedLine=0)
    RESPONSE Menu: M[Id=0x1010 P=0x0 N=0x0 B=0x1000 Text="Gyro settings"[0xF9]]
    SEND DSM_getFirstMenuLine(MenuId=0x1010)
    RESPONSE MenuLine: L[#0 T=M VId=0x1011 Text="AS3X Settings"[0x1DD]   MId=0x1010 ]
    SEND DSM_getNextLine(MenuId=0x1010,LastLine=0)
    RESPONSE MenuLine: L[#1 T=M VId=0x1019 Text="SAFE Settings"[0x1E2]   MId=0x1010 ]
    SEND DSM_getNextLine(MenuId=0x1010,LastLine=1)
    RESPONSE MenuLine: L[#2 T=M VId=0x1021 Text="F-Mode Setup"[0x87]   MId=0x1010 ]
    SEND DSM_getNextLine(MenuId=0x1010,LastLine=2)
    RESPONSE MenuLine: L[#3 T=M VId=0x1022 Text="System Setup"[0x86]   MId=0x1010 ]

## Menu 

The menu has the following information:

    Menu: M[Id=0x1010 P=0x0 N=0x0 B=0x1000 Text="Gyro settings"[0xF9]]

- `MenuId`: The menu ID number of the menu (hex, 16 bit number)
- `PrevId`: The menu ID of the previous menu (for navigation), Log show as `"P="`
- `NextId`: The menu ID of the next menu (for navigation), Log shows as `"N="`
- `BackId`: The menu ID of the back menu (for navigation), Log shows as `"B="`
- `TextId`: The message number to display (16 bits, Hex). Log shows as [`0xXX`] after the message.  
- `Text`: Retrived using the `TextId` from the script message `Text` array.

## Menu Lines

The menu lines has the following information:

    L[#0 T=V_nc VId=0x1000 Text="Flight Mode"[0x8001] Val=1 [0->10,0] MId=0x1021 ]
    L[#1 T=M VId=0x7CA6 Text="FM Channel"[0x78]   MId=0x1021 ]
    L[#2 T=LM VId=0x1002 Text="AS3X"[0x1DC] Val=1|"Act" NL=(0->1,0,S=3) [3->4,3] MId=0x1021 ]

-  `MenuId`: of the menu they beling to. Log show as `"MId="` at the end.
-  `LineNum`: Line number (0..5). The line number in the screen. Log show as # in the beginning
-  `Type`: Type of Line, Log shows as `"T="`  (explanation later)
-  `TextId`: The message number to display (16 bits, Hex). Log shows as [`0xXXXX`] after the message.  
-  `Text`:  Retrived using the `TextId` from the script message `Text` array.
-  `ValueId`: The value or menu ID of the line. Log shows as `"VId="` (16 bits, Hex).
-  `Value Range`: Shows as [`Min`->`Max`, `Default`]. This is the RAW data comming from the RX
-  `NL`: Computed Normalized LIST (0 reference) for List Values. Source is the RAW range. For example, for lines of list of values.   `[3->4,3]` is tranlated to `NL=(0->1,0,S=3)` since the value is also normalize to 0. `"S="` means the initial entry in the `List_Text` array
-  `Val`: Current value for line who hold data. Relative to 0 for List Values. For List Values, the log will show the translation of the value to display text. example: `Val=1|"Act"` that is coming from `List_Value[4]`

## Type of Menu Lines

-   `LINE_TYPE.MENU (Log: "T=M")`: This could be regular text or a navigation to another menu. if `ValueId` is the same as the current MenuId (`MId=`), is a plain text line (navigation to itself).  If the `ValueId` is not the current menuId, then `ValueId` is the MenuId to navigate to.  

    We have found only one exception to the plain text rule, a true navigation to itself, in that case, in the text of the menu, you can use the "/M" flag at the end of the text to force it to be a menu button. 

        Example, FM_Channel is a navigation to menuId=0x7CA6.

        L[#1 T=M VId=0x7CA6 Text="FM Channel"[0x78]   MId=0x1021 ]

-   `LINE_TYPE.LIST_MENU_NC (Log T=LM_nc)`:  This is a line that shows as text in the GUI. The numeric value is translated to the proper text. The range is important, since it descrives the range of posible values. No incremental RX changes, only at the end.

        Example: List of Values, List_Text[] starts at 53, ends at 85, with a default of 85. When normalized to 0, is a range from 0->32 for the numeric value. The Display value `Aux1` is retrive from `List_Text[6+53]`.

        L[#0 T=LM_nc VId=0x1000 Text="FM Channel"[0x78] Val=6|"Aux1" NL=(0->32,0,S=53) [53->85,53] MId=0x7CA6 ]

 -  `LINE_TYPE.LIST_MENU_TOG (Log T=L_tog)`: Mostly the same as LIST_MENU_NC, but is just 2 values. (ON/OFF, Ihn/Act, etc). Should be a toggle in the GUI.

        L[#2 T=LM_tog VId=0x1002 Text="AS3X"[0x1DC] Val=1|"Act" NL=(0->1,0,S=3) [3->4,3] MId=0x1021 ]

-  `LINE_TYPE.LIST_MENU (Log T=LM)`: Mostly the same as LIST_MENU_NC, but incremental changes to the RX. Some times, it comes with a strange range `[0->244,Default]`. Usually this means that the values are not contiguos range; usually Ihn + Range. Still haven't found where in the data the correct range comes from. 

        Example: Valid Values: 3, 176->177 (Inh, Self-Level/Angle Dem, Envelope)
        L[#3 T=LM VId=0x1003 Text="Safe Mode"[0x1F8] Val=176|"Self-Level/Angle Dem" NL=(0->244,3,S=0) [0->244,3] MId=0x1021 ]

-   `LINE_TYPE.VALUE_NUM_I8_NC (Log: "T=V_nc")`: This line is editable, but is not updated to the RX incrementally, but only at the end. The Flight Mode line is of this type, so we have to check the TextId to differenciate between Flight mode and an Editable Value.  
Fligh Mode TextId is between 0x8000 and 0x8003

        Example, Flight mode comes from Variable ValId=0x1000, with current value of 1. Range of the Value is 0..10.

        L[#0 T=V_nc VId=0x1000 Text="Flight Mode"[0x8001] Val=1 [0->10,0] MId=0x1021 ]


-   `LINE_TYPE.VALUE_NUM_I8 (Log T=V_i8)`:  8 bit number (1 byte) 
-   `LINE_TYPE.VALUE_NUM_I16' (Log T=V_i16)`: 16 Bit number (2 bytes)
-   `LINE_TYPE.VALUE_NUM_SI16 (Log T=V_si16)`: Signed 16 bit number (2 bytes) 
-   `LINE_TYPE.VALUE_PERCENT (Log T=L_%)`: Shows a Percent Value. 1 Byte value.
 -  `LINE_TYPE.VALUE_DEGREES (Log T=L_de)`: Shows a Degrees VAlue. 1 Byte value.


## LIST_TYPE Bitmap
TYPE|Sum|Hex|7 Signed|6 Valid Min/Max??|5 No-Inc-Changing|4 Menu|3 List-Menu|2 text / number|1|0 - 16 bits
|-|-|-|-|-|-|-|-|-|-|-
|MENU|Text|0x1C|0|0|0|1|1|1|0|0
|LIST_MENU|Text|0x0C|0|0|0|0|1|1|0|0
|LIST_MENU_TOG|Text|0x4C|0|1|0|0|1|1|0|0
|LIST_MENU_NC|Text, NC|0x6C|0|1|1|0|1|1|0|0
|VALUE_NUM_I8_NC|I8, NC|0x60|0|1|1|0|0|0|0|0
|VALUE_PERCENT|S8|0xC0|1|1|0|0|0|0|0|0
|VALUE_DEGREES|S8 NC|0xE0|1|1|1|0|0|0|0|0
|VALUE_NUM_I8|I8|0x40|0|1|0|0|0|0|0|0
|VALUE_NUM_I16|I16|0x41|0|1|0|0|0|0|0|1
|VALUE_NUM_SI16|S16|0xC1|1|1|0|0|0|0|0|1


## Important Behavioral differences when updating values

Values who are editable, are updated to RX as they change. For example, when changing attitude trims, the servo moves as we change the value in real-time.

LIST_MENU_NC, VALUE_NUM_I8_NC don't update the RX as it changes. It changes only in the GUI, and only update the RX at the end when confirmed the value.  (NO-INC-CHANGES Bit)

After finishing updating a value, a validation command is sent. RX can reject the current value, and will change it to the nearest valid value.

## Special Menus

Seems like menuId=0x0001 is special. When you navigate to this menu, the RX reboots.
When this happens, we need to start from the beginning as if it was a new connection.

# Send and Receive messages

To comunicate with the Multi-Module, Lua scripts in OpenTx/EdgeTx has access to the `Multi_Buffer`. Writting to it will send data to RX, received data will be read from it.

For our specific case, this is how the Multi_Buffer is used:

|0..2|3|4..9|10..25
|--|--|--|--
|DSM|0x70+len|TX->RX data|RX->TX Data

To write a new DSM Fwd Programing command, write the data to address 4..9, and later set the address 3 with the length.  

When receiving data, address 10 will have the message type we are receiving, or 0 if nothing has been received.

## Starting a new DSM Forward programming Connection 

- Write 0x00 at address 3
- Write 0x00 at address 10
- Write "DSM" at address 0..2

## Disconnect

- Write 0x00 at address 0


# Request Messages (TX->RX)

## DSM_sendHeartbeat()
keep connection open.. We need to send it every 2-3 seconds, otherwise the RX will force close the connection by sending the TX an Exit_Confirm message.

|4|5|6|7|8|9|10
|--|--|--|--|--|--|--
Msg| Len? | ?? | ?? 
0x00|0x04|0x00|0x00
    
    SEND DSM_sendHeartbeat()
    DSM_SEND: [00 04 00 00 ]

 ## DSM_getRxVersion()
Request the RX information

|4|5|6|7|8|9|10
|--|--|--|--|--|--|--
Msg| Len? | ?? | ?? |??|??
0x11|0x06|0x00|0x14|0x00|0x00

    SEND DSM_getRxVersion() 
    DSM_SEND: [11 06 00 14 00 00 ]

## DSM_getMainMenu()
Request data for the main menu of the RX

|4|5|6|7|8|9|10
|--|--|--|--|--|--|--
Msg| Len? | ?? | ?? |??|??
0x12|0x06|0x00|0x14|0x00|0x00

    SEND DSM_getMainMenu()
    DSM_SEND: [12 06 00 14 00 00 ]


## DSM_getMenu(menuId, lastSelLine)
Request data for Menu with ID=`menuId`. lastSelLine is the line that was selected to navigate to that menu. Most menus works with 0, but for some special "Enter Bind Mode", "Factory Reset", "Save to Backup" they will not work if we send 0, has to be the line who was selected in the confirmation menu line "Apply".

|4|5|6|7|8|9|10
|--|--|--|--|--|--|--
Msg|Len? | MSB (menuId) | LSB (MenuId) | MSB (line#)??| LSB (line#)
0x16|0x06|0x10|0x60|0x00|0x01

    SEND DSM_getMenu(MenuId=0x1060 LastSelectedLine=1)
    DSM_SEND: [16 06 10 60 00 01 ]

## DSM_getFirstMenuLine(menuId)
Request the first line of a menu identified as `menuId`. The response will be the first line of the menu. Some times, it return lines shown as `'MenuUknownLine_0x05'` that we still are trying to understand what they are for.

|4|5|6|7|8|9|10
|--|--|--|--|--|--|--
Msg|Len? | MSB (menuId) | LSB (MenuId) 
0x13|0x04|0x10|0x60

    SEND DSM_getFirstMenuLine(MenuId=0x1000)
    DSM_SEND: [13 04 10 00 ]

## DSM_getNextMenuLine(menuId, curLine)
Request the retrival of the next line following the current line. Response is either the next line, or the next value, or nothing.

|4|5|6|7|8|9|10
|--|--|--|--|--|--|--
Msg|Len? | MSB (menuId) | LSB (MenuId) | MSB (line#)??| LSB (line#)
0x14|0x06|0x10|0x60|0x00|0x01

    SEND DSM_getNextLine(MenuId=0x1000,LastLine=1)
    DSM_SEND: [14 06 10 00 00 01 ]

##  DSM_getNextMenuValue(menuId, valId, text)
Retrive the next value after the last `ValId` of the current `menuId`.  text is just for debugging purposes to show the header of the value been retrived.
The Response is a Menu Value or nothing if no more data.

|4|5|6|7|8|9|10
|--|--|--|--|--|--|--
Msg|Len? | MSB (menuId) | LSB (MenuId) | MSB (ValId)| LSB (ValId)
0x15|0x06|0x10|0x61|0x10|0x00

    SEND DSM_getNextMenuValue(MenuId=0x1061, LastValueId=0x1000) Extra: Text="Outputs"
    DSM_SEND: [15 06 10 61 10 00 ]

## DSM_updateMenuValue(valId, val, text, line)
Updates the value identified as `valId` with the numeric value `val`. `text` and `line` are there to add debugging info.  No response is expected.

If the value is negative, it has to be translated to the proper DSM negative representaion.

|4|5|6|7|8|9|10
|--|--|--|--|--|--|--
Msg|Len? | MSB (ValId) | LSB (ValId) | MSB (Value)| LSB (Value)
0x18|0x06|0x??|0x??|0x??|0x??

    DSM_updateMenuValue(valId, val, text, line)
    -->DSM_send(0x18, 0x06, int16_MSB(valId), int16_LSB(valId), int16_MSB(value), int16_LSB(value)) 

## DSM_validateMenuValue(valId, text, line)
Validates the value identified as `valId`. `text` and `line` are there to add debugging info.  The RX can response an Update value if the value is not valid and needs to be corrected.

|4|5|6|7|8|9|10
|--|--|--|--|--|--|--
Msg|Len? | MSB (ValId) | LSB (ValId)
0x19|0x06|0x??|0x??


    DSM_validateMenuValue(valId, text, line)
    -> DSM_send(0x19, 0x06, int16_MSB(valId), int16_LSB(valId)) 

## DSM_menuValueChangingWait(valId, text, line) 
Durin editing, this serves as a heartbeat that we are editing the value. The value identified as `valId`. `text` and `line` are there to add debugging info.  The RX can response an Update value or a NUL response.

|4|5|6|7|8|9|10
|--|--|--|--|--|--|--
Msg|Len?? | MSB (ValId) | LSB (ValId)
0x1A|0x06|0x??|0x??

    DSM_menuValueChangingWait(valId, text, line)
    ->DSM_send(0x1A, 0x06, int16_MSB(valId), int16_LSB(valId))

## DSM_exitRequest()
Request to end the DSM Frd Prog connection. Will reponse with an exit confirmation.

|4|5|6|7|8|9|10
|--|--|--|--|--|--|--
Msg|Len?? | ??
0x1F|0x02|0xAA

    CALL DSM_exitRequest()
    DSM_SEND: [1F 02 AA ]

# Response Messages (RX->TX)

All responses will have the a response byte in Multi_Buffer[10]=0x09, and the type of message in Multi_Buffer[11].

## RX Version Response

Returns the information about the current RX.

The Display text of name name of the RX is retrive from the `RX_Name` array.

|10|11|12|13|14|15|16
|--|--|--|--|--|--|--
|Resp|Msg|?? |RxId|Major|Minor|Patch
|0x09|0x01|0x00|0x1E|0x02|0x26|0x05

    RESPONSE RX: 09 01 00 1E 02 26 05 
    RESPONSE Receiver=AR631 Version 2.38.5

## Menu Response
Returns the menu information to display and navigation.
The Display text for the menu is retrive from the `Text` array.


|10|11|12|13|14|15|16|17|18|19|20|21
|--|--|--|--|--|--|--|--|--|--|--|--
|Resp|Msg|LSB (menuId)|MSB (menuId)|LSB (TextId)|MSB (TextId)|LSB (PrevId)|MSB (PrevId)|LSB (NextId)|MSB (NextId)|LSB (BackId)|MSB (BackId)
|0x09|0x02|0x5E|0x10|0x27|0x02|0x00|0x00|0x00|0x00|0x00|0x10

    RESPONSE RX: 09 02 5E 10 27 02 00 00 00 00 00 10 00 00 00 00 
    RESPONSE Menu: M[Id=0x105E P=0x0 N=0x0 B=0x1000 Text="Other settings"[0x227]]



## Menu Line Response
Returns the menu line information.

The Display text for the menu line is retrive from the `Text` array.
 `Min`,`Max` and `Default` can be signed numbers.

|10|11|12|13|14|15|16|17|18|19|20|21|22|23|24|25
|--|--|--|--|--|--|--|--|--|--|--|--|--|--|--|--
|Resp|Msg|LSB (menuId)|MSB (menuId)|Line#|Line Type|LSB (TextId)|MSB (TextId)|LSB (ValId)|MSB (ValId)|LSB (Min)|MSB (Min)|LSB (Max)|MSB (Max)|LSB (Def)|MSB (Def)
|0x09|0x03|0x61|0x10|0x00|0x6C|0x50|0x00|0x00|0x10|0x36|0x00|0x49|0x00|0x36|0x00

    RESPONSE RX: 09 03 61 10 00 6C 50 00 00 10 36 00 49 00 36 00 
    RESPONSE MenuLine: L[#0 T=LM_nc VId=0x1000 Text="Outputs"[0x50] Val=nil NL=(0->19,0,S=54) [54->73,54] MId=0x1061 ]

## Menu Line Value Response
Returns the Value for a line. 

The response updates the Value in the line identified by `ValId`.
The Display text for the Value, when it is a list, is retrive from the `List_Text` array.

|10|11|12|13|14|15|16|17
|--|--|--|--|--|--|--|--
|Resp|Msg|LSB (menuId)|MSB (menuId)|LSB (ValId)|MSB (ValId)|LSB (Value)|MSB (Value)
|0x09|0x04|0x61|0x10|0x00|0x10|0x00|0x00

    RESPONSE RX: 09 04 61 10 00 10 00 00 
    RESPONSE MenuValue: UPDATED: L[#0 T=L_m0 VId=0x1000 Text="Outputs"[0x50] Val=0|"Throttle" NL=(0->19,0,S=54) [54->73,54] MId=0x1061 ]

## Exit Response
Response from a Exit Request.

|10|11
|--|--
|Resp|Msg
|0x09|0x07

    RESPONSE RX: 09 A7  
    RESPONSE Exit Confirm

## NULL Response
Can be use as a response, or heartbeat from the RX to keep the connection open.

|10|11
|--|--
|Resp|Msg
|0x09|0x00

    RESPONSE RX: 09 00  
    RESPONSE NULL


# Unknown Lines
TOTALLY UNKNOWN WHAT THIS ARE FOR.. but only works for the Main Menu..
Other menus they just loop on line=0 forever.

## DSM_getNextUknownLine_0x05(menuId, curLine)


Request the retrival of the next Unknown line following the current line. Response is either the next unknow line, next menu line, or the next value, or nothing.

|4|5|6|7|8|9| Comment
|--|--|--|--|--|--|--
Msg|Len? | Line# | Line# | 0x00 | Formula(line#)??
0x20|0x06|0x00|0x00|0x00|0x40 | LastLineLine=0 retrieval
0x20|0x06|0x01|0x01|0x00|0x01| LastLineLine=1 retrieval
0x20|0x06|0x02|0x02|0x00|0x02| LastLineLine=2 retrieval
0x20|0x06|0x03|0x03|0x00|0x04| LastLineLine=3 retrieval
0x20|0x06|0x04|0x04|0x00|0x00| LastLineLine=4 retrieval
0x20|0x06|0x05|0x05|0x00|0x00| LastLineLine=5 retrieval

## Unknown Line Response
We still don't know what is this for, but we have to retrive them and skip then. Works for main menu, but when it happens in another menus, usually we stay in an infinite loop retrieving line=0

|10|11|12|13|14|15|16|17
|--|--|--|--|--|--|--|--
|Resp|Msg|LSB (line#)
|0x09|0x05|0x00|0x01|0x00|0x00|0x00|0x07
|0x09|0x05|0x01|0x01|0x00|0x00|0x00|0x07

## Interaction on Main Menu
This is the normal interaction for the main menu. As you can see, it iterates on the 6 Unknow lines (0..5), and afterwards, it starts sending normal menu lines.

    SEND DSM_getFirstMenuLine(MenuId=0x1000)
    RESPONSE MenuUknownLine_0x05: LineNum=0  DATA=RX: 09 05 00 01 00 00 00 07 00 00 00 00 00 00 00 00
    CALL DSM_getNextUknownLine_0x05(LastLine=0)
    RESPONSE MenuUknownLine_0x05: LineNum=1  DATA=RX: 09 05 01 01 00 00 00 07 00 00 00 00 00 00 00 00
    CALL DSM_getNextUknownLine_0x05(LastLine=1)
    RESPONSE MenuUknownLine_0x05: LineNum=2  DATA=RX: 09 05 02 01 00 00 00 07 00 00 00 00 00 00 00 00
    CALL DSM_getNextUknownLine_0x05(LastLine=2)
    RESPONSE MenuUknownLine_0x05: LineNum=3  DATA=RX: 09 05 03 01 00 00 00 07 00 00 00 00 00 00 00 00
    CALL DSM_getNextUknownLine_0x05(LastLine=3)
    RESPONSE MenuUknownLine_0x05: LineNum=4  DATA=RX: 09 05 04 01 00 00 00 07 00 00 00 00 00 00 00 00
    CALL DSM_getNextUknownLine_0x05(LastLine=4)
    RESPONSE MenuUknownLine_0x05: LineNum=5  DATA=RX: 09 05 05 01 00 00 00 07 00 00 00 00 00 00 00 00
    CALL DSM_getNextUknownLine_0x05(LastLine=5)
    RESPONSE MenuLine: L[#0 T=M VId=0x1010 Text="Gyro settings"[0xF9]   MId=0x1000 ]

## Other menus
If it hapen on other menus. Usualy stays in an infinite loop until it crash/exits.
The screen will show **"Error: Cannot Load Menu Lines from RX"**

The log will look like:

    DSM_getMenu(MenuId=0x104F LastSelectedLine=1)
    RESPONSE Menu: M[Id=0x104F P=0x0 N=0x0 B=0x1000 Text="First Time Setup"[0x4A]]
    SEND DSM_getFirstMenuLine(MenuId=0x104F)
    RESPONSE MenuUknownLine_0x05: LineNum=0  DATA=RX: 09 05 00 01 00 00 00 07 00 00 00 00 00 00 00 00
    CALL DSM_getNextUknownLine_0x05(LastLine=0)
    RESPONSE MenuUknownLine_0x05: LineNum=0  DATA=RX: 09 05 00 01 00 00 00 07 00 00 00 00 00 00 00 00
    ERROR: Received Same menu line
    CALL DSM_getNextUknownLine_0x05(LastLine=0)
    RESPONSE MenuUknownLine_0x05: LineNum=0  DATA=RX: 09 05 00 01 00 00 00 07 00 00 00 00 00 00 00 00
    ERROR: Received Same menu line

We found that sometimes, Overriding LastSelectedLine to 0 solves the problem for some specific menus. Not for all (for other, is the oposite (0->1)). But at least no unknown lines are returned with this hack for AR631/AR637. Maybe others also needed.

**Overriding to Zero is not a good solution for every menu. Some menus needs the LastLine to know the behaviour (for example, Factory Reset the RX, Save Backup, Restore Backup, Enter Bind Mode, Some sensor Calibration). Thats why we cannot do it blindly.**

Here is the current code to fix some of this problems in AR631/AR637.
Function `DSM_SelLine_HACK()`

    if (ctx.RX.Id == RX.AR637T or ctx.RX.Id == RX.AR637TA or ctx.RX.Id == RX.AR631) then
        -- AR631/AR637 Hack for "First time Setup" or 
        -- "First Time AS3X Setup", use 0 instead of the ctx.SelLine=5
        if (ctx.Menu.MenuId == 0x104F  or ctx.Menu.MenuId==0x1055) then
            LOG_write("First time Setup Menu HACK: Overrideing LastSelectedLine to ZERO\n")
            ctx.SelLine = 0
        end
        -- DID NOT WORK: AR631/AR637 Hack for "Relearn Servo Settings", use 1 instead 
        -- of the ctx.SelLine=0
        --if (ctx.Menu.MenuId == 0x1023) then
        --    LOG_write("Relearn Servo Settings HACK: Overrideing LastSelectedLine to 1\n")
        --    ctx.SelLine = 1
        --end

Now it retrives properly the menu:

 Log shows:
    
    First time Setup Menu HACK: Overrideing LastSelectedLine to ZERO
    DSM_getMenu(MenuId=0x104F LastSelectedLine=0)
    RESPONSE Menu: M[Id=0x104F P=0x0 N=0x0 B=0x105E Text="First Time Setup"[0x4A]]
    SEND DSM_getFirstMenuLine(MenuId=0x104F)
    .. Good menu data





