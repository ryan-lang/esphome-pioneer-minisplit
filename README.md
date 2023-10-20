# esphome-pioneer-minisplit
Custom component for Pioneer WYT Inverter Series Mini Split / Heat Pump. Mine is the 12000 BTU version.

Took apart a [Pioneer Wireless Control Module (Diamante Series)](https://www.pioneerminisplit.com/products/wireless-internet-access-control-module-for-pioneer-diamante-series-systems) to find a Tuya TYWE1S + TYJW2 v2.0.0, which is a repackaged ESP8266EX. Flashed via USB to Serial.

It appears to talk the same language as some other differently-branded AC units (see htmltiger repo below). Most commands & status reports are identified. 3 or 4 are still a mystery. Help is appreciated!

Much help provided by the work of htmltiger:
https://github.com/htmltiger/tcl-electriq-split-ac
