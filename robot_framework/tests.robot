*** Settings ***
Library  SerialLibrary

*** Variables ***
${com}   COM3
${board}   nRF

*** Test Cases ***
Connect Serial
	Log To Console  Connecting to ${board}
	Add Port  ${com}  baudrate=115200   encoding=ascii
	Port Should Be Open  ${com}
	Reset Input Buffer
	Reset Output Buffer
	Sleep    1s
Turn On Debug

    Log To Console  Turning Debug ON
    DebugON/OFF
	${read} =   Read Until  terminator=\n  encoding=ascii
	Should Be Equal As Strings        ${read}    DEBUG prints enabled    strip_spaces=True
	Sleep    1s
Yellow Mode
	Log To Console  Yellow on indefinitely
	YellowBlink
	${read} =   Read Until  terminator=${\n}  encoding=ascii
	Should Be Equal As Strings        ${read}    YELLOW_debug    strip_spaces=True
	Sleep    5s
Force Stop Any Mode
	Log To Console  Forcing lights of in 3s
	Force Stop
	${read} =   Read Until  terminator=${\n}  encoding=ascii
	Should Be Equal As Strings        ${read}    !OFF_debug    strip_spaces=True
	Sleep    2s
LED ON
	Turn Red Led On 4s
	${read} =   Read Until  terminator=${\n}  encoding=ascii
	Should Be Equal As Strings        ${read}    COMMAND_OK    strip_spaces=True
	Log To Console  Sleeping for 5s
	Sleep    5s
TRAFFIC Mode
    Log To Console    Turning ON Traffic Light Mode
	TrafficMode
	${read} =   Read Until  terminator=${\n}  encoding=ascii
	Should Be Equal As Strings        ${read}    !TRAFFIC_debug    strip_spaces=True
	Sleep    12s
Green Blink
    Log To Console    Green Blink for 5 seconds
	Green Blink 
	Sleep    6s
	${read} =   Read Until  terminator=${\n}  encoding=ascii
	Should Be Equal As Strings        ${read}    COMMAND_OK    strip_spaces=True
Turn Off Debug
    Log To Console  Turning Debug OFF, ending test
	DebugON/OFF
	${read} =   Read Until  terminator=${\n}  encoding=ascii
	Should Be Equal As Strings        ${read}    DEBUG prints disabled    strip_spaces=True


Disconnect Serial
	Log To Console  Disconnecting ${board}
	[TearDown]  Delete Port  ${com}

*** Keywords ***


DebugON/OFF
    Log To Console    Debug Toggle
	Write Data    ?DEBUG\n       encoding=ascii
YellowBlink
    Log To Console    Blinking Yellow indefinitely
	Write Data    !YELLOW\n        encoding=ascii
TrafficMode
    Log To Console    Regular traffic lights
	Write Data    !TRAFFIC\n    encoding=ascii
Turn Red Led On 4s
	Log To Console    Set Red Led ON\n
	Write Data    RED,4,ON\n        encoding=ascii
Force Stop
    Log To Console    Stopping current light program
	Write Data    !OFF\n        encoding=ascii

Green Blink
	Write Data    GREEN,5,BLINK\n    encoding=ascii
	   
