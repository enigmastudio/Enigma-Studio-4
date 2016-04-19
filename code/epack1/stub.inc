; * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
;
;   This file is part of
;       ______        _                             __ __
;      / ____/____   (_)____ _ ____ ___   ____ _   / // /
;     / __/  / __ \ / // __ `// __ `__ \ / __ `/  / // /_
;    / /___ / / / // // /_/ // / / / / // /_/ /  /__  __/
;   /_____//_/ /_//_/ \__, //_/ /_/ /_/ \__,_/     /_/.   
;                    /____/                              
;
;   Copyright © 2003-2012 Brain Control, all rights reserved.
;
; * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *

ePeIatThunk struct
    dd  funcAddr
ePeIatThunk ends

ePeIltThunk typedef ePeIatThunk

ePeImportDesc struct
    dd  orgFirstThunk ; characteristics
    dd  timeDateStamp
    dd  forwarderChain
    dd  funcName
    dd  firstThunk
ePeImportDesc ends