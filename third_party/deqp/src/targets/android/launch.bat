@echo off
adb %* forward tcp:50016 tcp:50016
adb %* shell setprop log.tag.dEQP DEBUG
adb %* shell am start -n com.drawelements.deqp/.execserver.ServiceStarter
