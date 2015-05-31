power
=====

Power meter monitoring directly on the Pi using just an LDR. Add your own monitoring system (I used Zabbix).

I've forked this from https://github.com/kieranc/power, which in turn was forked from https://github.com/yfory/power .

Kieranc's code interfaces to a larger system - "Open Energy Monitor". If that's what you need to do, I suggest you check his code.

I've used the electronics from Kieranc's project. There's a transistor based circuit which provides a digital on/off signal.

# Requirements
* Raspberry Pi
* Photoresistor (Light Dependent Resistor)
* 10k Ohm resistor
* 1k resistors
* NPN Transistor - I used a BC547
* Relevant Cables/Connectors
* Modern Electricity Meter

Modern electricity meters have a blinking/flashing LED, often with small text that reads 1000 Imp/kWh. The two important things here are that you have a blinking LED, and that you know the number e.g. 800. Without these, this project will not work for you.

This project uses the LDR as one half of a voltage divider to trigger a transistor which is connected to a GPIO pin on the Pi.

The circuit is documented here: http://pyevolve.sourceforge.net/wordpress/?p=2383

I used a 1k resistor between base and ground to make it more sensitive and later a 2k2 potentiometer in its place to provide some adjustment but I've not had to adjust it. 1k should be fine.

# Software Installation

The file named power-monitor is used to automatically start the data logging process on boot and stop on shutdown. For testing purposes, you do not need this script. However, you should make use of it if you are setting up a more permanent solution.

```bash
sudo cp power-monitor /etc/init.d/
sudo chmod a+x /etc/init.d/power-monitor
sudo update-rc.d power-monitor defaults
```

The application will count for interrupts on the specified GPIO pin. This number is then exposed via TCP port 1001, and reset to zero on each successful TCP send.

```bash
gcc gpio-irq-demo.c -o gpio-irq -lpthreads
```

Put it somewhere accessible - I used /bin.

Once all this is done you can start the process...

```bash
sudo /etc/init.d/power-monitor start
```

You can test it easily with nc, which should return some number.
```bash
nc localhost 1001
```


# License

Copyright (c) 2012 Edward O'Regan

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

