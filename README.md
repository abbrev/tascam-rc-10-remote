TASCAM RC-10 remote control
===========================

This is a replacement for a TASCAM RC-10 remote control running on
Arduino.

Protocol
--------

The RC-10 communicates over a 2.5mm stereo headphone plug. The tip is
the signal from the remote control to the device under control (such as
an audio recorder), the ring is 3.3V DC from the device, and the sleeve
is ground.

The protocol is standard TTL-level UART (mark is 3.3V, space is 0V)
running at 9600 baud, 8 data bits, even parity, and 1 stop bit.

The RC-10 sends a start byte when a button is pressed, a repeat byte
every 100ms afterward while the button is held down, and an end byte when
the button is released. The lower 5 bits of the byte is the command,
while the upper 2 bits indicate whether it's a start, repeat, or end
byte. A start byte has bit 7 set and bit 6 cleared, a repeat byte has
both bits 7 and 6 set, and an end byte has both bits 7 and 6 cleared.

Button command values
---------------------

| Button  | Value |
|---------|-------|
| Stop    |     8 |
| Play    |     9 |
| Record  |    11 |
| Forward |    14 |
| Back    |    15 |
| Mark    |    24 |
| F1      |    28 |
| F2      |    29 |
| F3      |    30 |
| F4      |    31 |

Note: F3 is marked (+) and F4 is marked (-).

RC-10 button layout
-------------------

The RC-10 has the following button layout:

    (Back) (Forw)
    (Stop) (Play)
    (Reco) (Mark)
    (F1  ) (F3  )
    (F2  ) (F4  )

A better layout might look something like this:

    (F1  )  (F2  )
    (Stop)  (Play)  (Rec )
            (F3  )
     (Back) (Mark) (Forw)
            (F4  )

(This layout mimics its use with a TASCAM DR-40. Other devices may have
other layouts.)

Features
--------

This replacement software also supports a "turbo" mode which increases
the repeat rate of buttons to 4x the normal rate. This is useful for
impatient people. :) Turbo mode can be toggled with a press of the turbo
button.

Acknowledgments
---------------

Special thanks to user iamin on www.ghielectronics.com for posting
information about the RC-10 protocol.
(See https://www.ghielectronics.com/community/codeshare/entry/1062)
