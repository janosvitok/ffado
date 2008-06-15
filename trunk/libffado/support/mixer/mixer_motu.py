#
# Copyright (C) 2005-2008 by Pieter Palmers
# Copyright (C) 2008 by Jonathan Woithe
#
# This file is part of FFADO
# FFADO = Free Firewire (pro-)audio drivers for linux
#
# FFADO is based upon FreeBoB.
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 2 of the License, or
# (at your option) version 3 of the License.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#

from qt import *
from mixer_motuui import *

class MotuMixer(MotuMixerUI):
    def __init__(self,parent = None,name = None,fl = 0):
        MotuMixerUI.__init__(self,parent,name,fl)

    # public slot: channel/mix faders
    def updateFader(self, a0):
        sender = self.sender()
        vol = 128-a0
        print "setting %s channel/mix fader to %d" % (self.ChannelFaders[sender][0], vol)
        self.hw.setDiscrete(self.ChannelFaders[sender][0], vol)

    # public slot: a generic multivalue control
    def updateControl(self, a0):
        sender = self.sender()
        val = a0
        print "setting %s control to %d" % (self.Controls[sender][0], val)
        self.hw.setDiscrete(self.Controls[sender][0], val)

    # public slot: generic binary switch
    def updateBinarySwitch(self, a0):
        sender = self.sender()
        val=a0
        print "setting %s switch to %d" % (self.BinarySwitches[sender][0], val)
        self.hw.setDiscrete(self.BinarySwitches[sender][0], val)

    # public slot: mix destination control
    def updateMixDest(self, a0):
        sender = self.sender()
        dest=a0
        print "setting %s mix destination to %d" % (self.MixDests[sender][0], dest)
        self.hw.setDiscrete(self.MixDests[sender][0], dest)

    # public slots: mix output controls
    def set_mix1_dest(self,a0):
        self.setMixDest('mix1', a0)

    def setSelector(self,a0,a1):
        name=a0
        state = a1
        print "setting %s state to %d" % (name, state)
        self.hw.setDiscrete(self.SelectorControls[name][0], state)

    def init(self):
        print "Init MOTU mixer window"

        self.ChannelFaders={
            self.mix1ana1_fader: ['/Mixer/Mix1/Ana1_fader'],
            self.mix1ana2_fader: ['/Mixer/Mix1/Ana2_fader'],
            self.mix1ana3_fader: ['/Mixer/Mix1/Ana3_fader'],
            self.mix1ana4_fader: ['/Mixer/Mix1/Ana4_fader'],
            self.mix1ana5_fader: ['/Mixer/Mix1/Ana5_fader'],
            self.mix1ana6_fader: ['/Mixer/Mix1/Ana6_fader'],
            self.mix1ana7_fader: ['/Mixer/Mix1/Ana7_fader'],
            self.mix1ana8_fader: ['/Mixer/Mix1/Ana8_fader'],
            self.mix1adat1_fader: ['/Mixer/Mix1/Adat1_fader'],
            self.mix1adat2_fader: ['/Mixer/Mix1/Adat2_fader'],
            self.mix1adat3_fader: ['/Mixer/Mix1/Adat3_fader'],
            self.mix1adat4_fader: ['/Mixer/Mix1/Adat4_fader'],
            self.mix1adat5_fader: ['/Mixer/Mix1/Adat5_fader'],
            self.mix1adat6_fader: ['/Mixer/Mix1/Adat6_fader'],
            self.mix1adat7_fader: ['/Mixer/Mix1/Adat7_fader'],
            self.mix1adat8_fader: ['/Mixer/Mix1/Adat8_fader'],
            self.mix1aes1_fader:  ['/Mixer/Mix1/Aes1_fader'],
            self.mix1aes2_fader:  ['/Mixer/Mix1/Aes2_fader'],
            self.mix1spdif1_fader: ['/Mixer/Mix1/Spdif1_fader'],
            self.mix1spdif2_fader: ['/Mixer/Mix1/Spdif2_fader'],
            self.mix1_fader: ['/Mixer/Mix1/Mix_fader'],

            self.mix2ana1_fader: ['/Mixer/Mix2/Ana1_fader'],
            self.mix2ana2_fader: ['/Mixer/Mix2/Ana2_fader'],
            self.mix2ana3_fader: ['/Mixer/Mix2/Ana3_fader'],
            self.mix2ana4_fader: ['/Mixer/Mix2/Ana4_fader'],
            self.mix2ana5_fader: ['/Mixer/Mix2/Ana5_fader'],
            self.mix2ana6_fader: ['/Mixer/Mix2/Ana6_fader'],
            self.mix2ana7_fader: ['/Mixer/Mix2/Ana7_fader'],
            self.mix2ana8_fader: ['/Mixer/Mix2/Ana8_fader'],
            self.mix2adat1_fader: ['/Mixer/Mix2/Adat1_fader'],
            self.mix2adat2_fader: ['/Mixer/Mix2/Adat2_fader'],
            self.mix2adat3_fader: ['/Mixer/Mix2/Adat3_fader'],
            self.mix2adat4_fader: ['/Mixer/Mix2/Adat4_fader'],
            self.mix2adat5_fader: ['/Mixer/Mix2/Adat5_fader'],
            self.mix2adat6_fader: ['/Mixer/Mix2/Adat6_fader'],
            self.mix2adat7_fader: ['/Mixer/Mix2/Adat7_fader'],
            self.mix2adat8_fader: ['/Mixer/Mix2/Adat8_fader'],
            self.mix2aes1_fader:  ['/Mixer/Mix2/Aes1_fader'],
            self.mix2aes2_fader:  ['/Mixer/Mix2/Aes2_fader'],
            self.mix2spdif1_fader: ['/Mixer/Mix2/Spdif1_fader'],
            self.mix2spdif2_fader: ['/Mixer/Mix2/Spdif2_fader'],
            self.mix2_fader: ['/Mixer/Mix2/Mix_fader'],

            self.mix3ana1_fader: ['/Mixer/Mix3/Ana1_fader'],
            self.mix3ana2_fader: ['/Mixer/Mix3/Ana2_fader'],
            self.mix3ana3_fader: ['/Mixer/Mix3/Ana3_fader'],
            self.mix3ana4_fader: ['/Mixer/Mix3/Ana4_fader'],
            self.mix3ana5_fader: ['/Mixer/Mix3/Ana5_fader'],
            self.mix3ana6_fader: ['/Mixer/Mix3/Ana6_fader'],
            self.mix3ana7_fader: ['/Mixer/Mix3/Ana7_fader'],
            self.mix3ana8_fader: ['/Mixer/Mix3/Ana8_fader'],
            self.mix3adat1_fader: ['/Mixer/Mix3/Adat1_fader'],
            self.mix3adat2_fader: ['/Mixer/Mix3/Adat2_fader'],
            self.mix3adat3_fader: ['/Mixer/Mix3/Adat3_fader'],
            self.mix3adat4_fader: ['/Mixer/Mix3/Adat4_fader'],
            self.mix3adat5_fader: ['/Mixer/Mix3/Adat5_fader'],
            self.mix3adat6_fader: ['/Mixer/Mix3/Adat6_fader'],
            self.mix3adat7_fader: ['/Mixer/Mix3/Adat7_fader'],
            self.mix3adat8_fader: ['/Mixer/Mix3/Adat8_fader'],
            self.mix3aes1_fader:  ['/Mixer/Mix3/Aes1_fader'],
            self.mix3aes2_fader:  ['/Mixer/Mix3/Aes2_fader'],
            self.mix3spdif1_fader: ['/Mixer/Mix3/Spdif1_fader'],
            self.mix3spdif2_fader: ['/Mixer/Mix3/Spdif2_fader'],
            self.mix3_fader: ['/Mixer/Mix3/Mix_fader'],

            self.mix4ana1_fader: ['/Mixer/Mix4/Ana1_fader'],
            self.mix4ana2_fader: ['/Mixer/Mix4/Ana2_fader'],
            self.mix4ana3_fader: ['/Mixer/Mix4/Ana3_fader'],
            self.mix4ana4_fader: ['/Mixer/Mix4/Ana4_fader'],
            self.mix4ana5_fader: ['/Mixer/Mix4/Ana5_fader'],
            self.mix4ana6_fader: ['/Mixer/Mix4/Ana6_fader'],
            self.mix4ana7_fader: ['/Mixer/Mix4/Ana7_fader'],
            self.mix4ana8_fader: ['/Mixer/Mix4/Ana8_fader'],
            self.mix4adat1_fader: ['/Mixer/Mix4/Adat1_fader'],
            self.mix4adat2_fader: ['/Mixer/Mix4/Adat2_fader'],
            self.mix4adat3_fader: ['/Mixer/Mix4/Adat3_fader'],
            self.mix4adat4_fader: ['/Mixer/Mix4/Adat4_fader'],
            self.mix4adat5_fader: ['/Mixer/Mix4/Adat5_fader'],
            self.mix4adat6_fader: ['/Mixer/Mix4/Adat6_fader'],
            self.mix4adat7_fader: ['/Mixer/Mix4/Adat7_fader'],
            self.mix4adat8_fader: ['/Mixer/Mix4/Adat8_fader'],
            self.mix4aes1_fader:  ['/Mixer/Mix4/Aes1_fader'],
            self.mix4aes2_fader:  ['/Mixer/Mix4/Aes2_fader'],
            self.mix4spdif1_fader: ['/Mixer/Mix4/Spdif1_fader'],
            self.mix4spdif2_fader: ['/Mixer/Mix4/Spdif2_fader'],
            self.mix4_fader: ['/Mixer/Mix4/Mix_fader'],
            }

        self.Controls={
            self.mix1ana1_pan:   ['/Mixer/Mix1/Ana1_pan'],
            self.mix1ana2_pan:   ['/Mixer/Mix1/Ana2_pan'],
            self.mix1ana3_pan:   ['/Mixer/Mix1/Ana3_pan'],
            self.mix1ana4_pan:   ['/Mixer/Mix1/Ana4_pan'],
            self.mix1ana5_pan:   ['/Mixer/Mix1/Ana5_pan'],
            self.mix1ana6_pan:   ['/Mixer/Mix1/Ana6_pan'],
            self.mix1ana7_pan:   ['/Mixer/Mix1/Ana7_pan'],
            self.mix1ana8_pan:   ['/Mixer/Mix1/Ana8_pan'],
            self.mix1adat1_pan:  ['/Mixer/Mix1/Adat1_pan'],
            self.mix1adat2_pan:  ['/Mixer/Mix1/Adat2_pan'],
            self.mix1adat3_pan:  ['/Mixer/Mix1/Adat3_pan'],
            self.mix1adat4_pan:  ['/Mixer/Mix1/Adat4_pan'],
            self.mix1adat5_pan:  ['/Mixer/Mix1/Adat5_pan'],
            self.mix1adat6_pan:  ['/Mixer/Mix1/Adat6_pan'],
            self.mix1adat7_pan:  ['/Mixer/Mix1/Adat7_pan'],
            self.mix1adat8_pan:  ['/Mixer/Mix1/Adat8_pan'],
            self.mix1aes1_pan:   ['/Mixer/Mix1/Aes1_pan'],
            self.mix1aes2_pan:   ['/Mixer/Mix1/Aes2_pan'],
            self.mix1spdif1_pan:  ['/Mixer/Mix1/Spdif1_pan'],
            self.mix1spdif2_pan:  ['/Mixer/Mix1/Spdif2_pan'],

            self.mix2ana1_pan:   ['/Mixer/Mix2/Ana1_pan'],
            self.mix2ana2_pan:   ['/Mixer/Mix2/Ana2_pan'],
            self.mix2ana3_pan:   ['/Mixer/Mix2/Ana3_pan'],
            self.mix2ana4_pan:   ['/Mixer/Mix2/Ana4_pan'],
            self.mix2ana5_pan:   ['/Mixer/Mix2/Ana5_pan'],
            self.mix2ana6_pan:   ['/Mixer/Mix2/Ana6_pan'],
            self.mix2ana7_pan:   ['/Mixer/Mix2/Ana7_pan'],
            self.mix2ana8_pan:   ['/Mixer/Mix2/Ana8_pan'],
            self.mix2adat1_pan:  ['/Mixer/Mix2/Adat1_pan'],
            self.mix2adat2_pan:  ['/Mixer/Mix2/Adat2_pan'],
            self.mix2adat3_pan:  ['/Mixer/Mix2/Adat3_pan'],
            self.mix2adat4_pan:  ['/Mixer/Mix2/Adat4_pan'],
            self.mix2adat5_pan:  ['/Mixer/Mix2/Adat5_pan'],
            self.mix2adat6_pan:  ['/Mixer/Mix2/Adat6_pan'],
            self.mix2adat7_pan:  ['/Mixer/Mix2/Adat7_pan'],
            self.mix2adat8_pan:  ['/Mixer/Mix2/Adat8_pan'],
            self.mix2aes1_pan:   ['/Mixer/Mix2/Aes1_pan'],
            self.mix2aes2_pan:   ['/Mixer/Mix2/Aes2_pan'],
            self.mix2spdif1_pan:  ['/Mixer/Mix2/Spdif1_pan'],
            self.mix2spdif2_pan:  ['/Mixer/Mix2/Spdif2_pan'],

            self.mix3ana1_pan:   ['/Mixer/Mix3/Ana1_pan'],
            self.mix3ana2_pan:   ['/Mixer/Mix3/Ana2_pan'],
            self.mix3ana3_pan:   ['/Mixer/Mix3/Ana3_pan'],
            self.mix3ana4_pan:   ['/Mixer/Mix3/Ana4_pan'],
            self.mix3ana5_pan:   ['/Mixer/Mix3/Ana5_pan'],
            self.mix3ana6_pan:   ['/Mixer/Mix3/Ana6_pan'],
            self.mix3ana7_pan:   ['/Mixer/Mix3/Ana7_pan'],
            self.mix3ana8_pan:   ['/Mixer/Mix3/Ana8_pan'],
            self.mix3adat1_pan:  ['/Mixer/Mix3/Adat1_pan'],
            self.mix3adat2_pan:  ['/Mixer/Mix3/Adat2_pan'],
            self.mix3adat3_pan:  ['/Mixer/Mix3/Adat3_pan'],
            self.mix3adat4_pan:  ['/Mixer/Mix3/Adat4_pan'],
            self.mix3adat5_pan:  ['/Mixer/Mix3/Adat5_pan'],
            self.mix3adat6_pan:  ['/Mixer/Mix3/Adat6_pan'],
            self.mix3adat7_pan:  ['/Mixer/Mix3/Adat7_pan'],
            self.mix3adat8_pan:  ['/Mixer/Mix3/Adat8_pan'],
            self.mix3aes1_pan:   ['/Mixer/Mix3/Aes1_pan'],
            self.mix3aes2_pan:   ['/Mixer/Mix3/Aes2_pan'],
            self.mix3spdif1_pan:  ['/Mixer/Mix3/Spdif1_pan'],
            self.mix3spdif2_pan:  ['/Mixer/Mix3/Spdif2_pan'],

            self.mix4ana1_pan:   ['/Mixer/Mix4/Ana1_pan'],
            self.mix4ana2_pan:   ['/Mixer/Mix4/Ana2_pan'],
            self.mix4ana3_pan:   ['/Mixer/Mix4/Ana3_pan'],
            self.mix4ana4_pan:   ['/Mixer/Mix4/Ana4_pan'],
            self.mix4ana5_pan:   ['/Mixer/Mix4/Ana5_pan'],
            self.mix4ana6_pan:   ['/Mixer/Mix4/Ana6_pan'],
            self.mix4ana7_pan:   ['/Mixer/Mix4/Ana7_pan'],
            self.mix4ana8_pan:   ['/Mixer/Mix4/Ana8_pan'],
            self.mix4adat1_pan:  ['/Mixer/Mix4/Adat1_pan'],
            self.mix4adat2_pan:  ['/Mixer/Mix4/Adat2_pan'],
            self.mix4adat3_pan:  ['/Mixer/Mix4/Adat3_pan'],
            self.mix4adat4_pan:  ['/Mixer/Mix4/Adat4_pan'],
            self.mix4adat5_pan:  ['/Mixer/Mix4/Adat5_pan'],
            self.mix4adat6_pan:  ['/Mixer/Mix4/Adat6_pan'],
            self.mix4adat7_pan:  ['/Mixer/Mix4/Adat7_pan'],
            self.mix4adat8_pan:  ['/Mixer/Mix4/Adat8_pan'],
            self.mix4aes1_pan:   ['/Mixer/Mix4/Aes1_pan'],
            self.mix4aes2_pan:   ['/Mixer/Mix4/Aes2_pan'],
            self.mix4spdif1_pan:  ['/Mixer/Mix4/Spdif1_pan'],
            self.mix4spdif2_pan:  ['/Mixer/Mix4/Spdif2_pan'],

            self.ana1_trimgain:  ['/Mixer/Control/Ana1_trimgain'],
            self.ana2_trimgain:  ['/Mixer/Control/Ana2_trimgain'],
            self.ana3_trimgain:  ['/Mixer/Control/Ana3_trimgain'],
            self.ana4_trimgain:  ['/Mixer/Control/Ana4_trimgain'],
        }

        self.BinarySwitches={
            self.mix1ana1_mute:  ['/Mixer/Mix1/Ana1_mute'],
            self.mix1ana2_mute:  ['/Mixer/Mix1/Ana2_mute'],
            self.mix1ana3_mute:  ['/Mixer/Mix1/Ana3_mute'],
            self.mix1ana4_mute:  ['/Mixer/Mix1/Ana4_mute'],
            self.mix1ana5_mute:  ['/Mixer/Mix1/Ana5_mute'],
            self.mix1ana6_mute:  ['/Mixer/Mix1/Ana6_mute'],
            self.mix1ana7_mute:  ['/Mixer/Mix1/Ana7_mute'],
            self.mix1ana8_mute:  ['/Mixer/Mix1/Ana8_mute'],
            self.mix1ana1_solo:  ['/Mixer/Mix1/Ana1_solo'],
            self.mix1ana2_solo:  ['/Mixer/Mix1/Ana2_solo'],
            self.mix1ana3_solo:  ['/Mixer/Mix1/Ana3_solo'],
            self.mix1ana4_solo:  ['/Mixer/Mix1/Ana4_solo'],
            self.mix1ana5_solo:  ['/Mixer/Mix1/Ana5_solo'],
            self.mix1ana6_solo:  ['/Mixer/Mix1/Ana6_solo'],
            self.mix1ana7_solo:  ['/Mixer/Mix1/Ana7_solo'],
            self.mix1ana8_solo:  ['/Mixer/Mix1/Ana8_solo'],
            self.mix1adat1_mute: ['/Mixer/Mix1/Adat1_mute'],
            self.mix1adat2_mute: ['/Mixer/Mix1/Adat2_mute'],
            self.mix1adat3_mute: ['/Mixer/Mix1/Adat3_mute'],
            self.mix1adat4_mute: ['/Mixer/Mix1/Adat4_mute'],
            self.mix1adat5_mute: ['/Mixer/Mix1/Adat5_mute'],
            self.mix1adat6_mute: ['/Mixer/Mix1/Adat6_mute'],
            self.mix1adat7_mute: ['/Mixer/Mix1/Adat7_mute'],
            self.mix1adat8_mute: ['/Mixer/Mix1/Adat8_mute'],
            self.mix1adat1_solo: ['/Mixer/Mix1/Adat1_solo'],
            self.mix1adat2_solo: ['/Mixer/Mix1/Adat2_solo'],
            self.mix1adat3_solo: ['/Mixer/Mix1/Adat3_solo'],
            self.mix1adat4_solo: ['/Mixer/Mix1/Adat4_solo'],
            self.mix1adat5_solo: ['/Mixer/Mix1/Adat5_solo'],
            self.mix1adat6_solo: ['/Mixer/Mix1/Adat6_solo'],
            self.mix1adat7_solo: ['/Mixer/Mix1/Adat7_solo'],
            self.mix1adat8_solo: ['/Mixer/Mix1/Adat8_solo'],
            self.mix1aes1_mute:  ['/Mixer/Mix1/Aes1_mute'],
            self.mix1aes2_mute:  ['/Mixer/Mix1/Aes2_mute'],
            self.mix1aes1_solo:  ['/Mixer/Mix1/Aes1_solo'],
            self.mix1aes2_solo:  ['/Mixer/Mix1/Aes2_solo'],
            self.mix1spdif1_mute: ['/Mixer/Mix1/Spdif1_mute'],
            self.mix1spdif2_mute: ['/Mixer/Mix1/Spdif2_mute'],
            self.mix1spdif1_solo: ['/Mixer/Mix1/Spdif1_solo'],
            self.mix1spdif2_solo: ['/Mixer/Mix1/Spdif2_solo'],
            self.mix1_mute:      ['/Mixer/Mix1/Mix_mute'],

            self.mix2ana1_mute:  ['/Mixer/Mix2/Ana1_mute'],
            self.mix2ana2_mute:  ['/Mixer/Mix2/Ana2_mute'],
            self.mix2ana3_mute:  ['/Mixer/Mix2/Ana3_mute'],
            self.mix2ana4_mute:  ['/Mixer/Mix2/Ana4_mute'],
            self.mix2ana5_mute:  ['/Mixer/Mix2/Ana5_mute'],
            self.mix2ana6_mute:  ['/Mixer/Mix2/Ana6_mute'],
            self.mix2ana7_mute:  ['/Mixer/Mix2/Ana7_mute'],
            self.mix2ana8_mute:  ['/Mixer/Mix2/Ana8_mute'],
            self.mix2ana1_solo:  ['/Mixer/Mix2/Ana1_solo'],
            self.mix2ana2_solo:  ['/Mixer/Mix2/Ana2_solo'],
            self.mix2ana3_solo:  ['/Mixer/Mix2/Ana3_solo'],
            self.mix2ana4_solo:  ['/Mixer/Mix2/Ana4_solo'],
            self.mix2ana5_solo:  ['/Mixer/Mix2/Ana5_solo'],
            self.mix2ana6_solo:  ['/Mixer/Mix2/Ana6_solo'],
            self.mix2ana7_solo:  ['/Mixer/Mix2/Ana7_solo'],
            self.mix2ana8_solo:  ['/Mixer/Mix2/Ana8_solo'],
            self.mix2adat1_mute: ['/Mixer/Mix2/Adat1_mute'],
            self.mix2adat2_mute: ['/Mixer/Mix2/Adat2_mute'],
            self.mix2adat3_mute: ['/Mixer/Mix2/Adat3_mute'],
            self.mix2adat4_mute: ['/Mixer/Mix2/Adat4_mute'],
            self.mix2adat5_mute: ['/Mixer/Mix2/Adat5_mute'],
            self.mix2adat6_mute: ['/Mixer/Mix2/Adat6_mute'],
            self.mix2adat7_mute: ['/Mixer/Mix2/Adat7_mute'],
            self.mix2adat8_mute: ['/Mixer/Mix2/Adat8_mute'],
            self.mix2adat1_solo: ['/Mixer/Mix2/Adat1_solo'],
            self.mix2adat2_solo: ['/Mixer/Mix2/Adat2_solo'],
            self.mix2adat3_solo: ['/Mixer/Mix2/Adat3_solo'],
            self.mix2adat4_solo: ['/Mixer/Mix2/Adat4_solo'],
            self.mix2adat5_solo: ['/Mixer/Mix2/Adat5_solo'],
            self.mix2adat6_solo: ['/Mixer/Mix2/Adat6_solo'],
            self.mix2adat7_solo: ['/Mixer/Mix2/Adat7_solo'],
            self.mix2adat8_solo: ['/Mixer/Mix2/Adat8_solo'],
            self.mix2aes1_mute:  ['/Mixer/Mix2/Aes1_mute'],
            self.mix2aes2_mute:  ['/Mixer/Mix2/Aes2_mute'],
            self.mix2aes1_solo:  ['/Mixer/Mix2/Aes1_solo'],
            self.mix2aes2_solo:  ['/Mixer/Mix2/Aes2_solo'],
            self.mix2spdif1_mute: ['/Mixer/Mix2/Spdif1_mute'],
            self.mix2spdif2_mute: ['/Mixer/Mix2/Spdif2_mute'],
            self.mix2spdif1_solo: ['/Mixer/Mix2/Spdif1_solo'],
            self.mix2spdif2_solo: ['/Mixer/Mix2/Spdif2_solo'],
            self.mix2_mute:      ['/Mixer/Mix2/Mix_mute'],

            self.mix3ana1_mute:  ['/Mixer/Mix3/Ana1_mute'],
            self.mix3ana2_mute:  ['/Mixer/Mix3/Ana2_mute'],
            self.mix3ana3_mute:  ['/Mixer/Mix3/Ana3_mute'],
            self.mix3ana4_mute:  ['/Mixer/Mix3/Ana4_mute'],
            self.mix3ana5_mute:  ['/Mixer/Mix3/Ana5_mute'],
            self.mix3ana6_mute:  ['/Mixer/Mix3/Ana6_mute'],
            self.mix3ana7_mute:  ['/Mixer/Mix3/Ana7_mute'],
            self.mix3ana8_mute:  ['/Mixer/Mix3/Ana8_mute'],
            self.mix3ana1_solo:  ['/Mixer/Mix3/Ana1_solo'],
            self.mix3ana2_solo:  ['/Mixer/Mix3/Ana2_solo'],
            self.mix3ana3_solo:  ['/Mixer/Mix3/Ana3_solo'],
            self.mix3ana4_solo:  ['/Mixer/Mix3/Ana4_solo'],
            self.mix3ana5_solo:  ['/Mixer/Mix3/Ana5_solo'],
            self.mix3ana6_solo:  ['/Mixer/Mix3/Ana6_solo'],
            self.mix3ana7_solo:  ['/Mixer/Mix3/Ana7_solo'],
            self.mix3ana8_solo:  ['/Mixer/Mix3/Ana8_solo'],
            self.mix3adat1_mute: ['/Mixer/Mix3/Adat1_mute'],
            self.mix3adat2_mute: ['/Mixer/Mix3/Adat2_mute'],
            self.mix3adat3_mute: ['/Mixer/Mix3/Adat3_mute'],
            self.mix3adat4_mute: ['/Mixer/Mix3/Adat4_mute'],
            self.mix3adat5_mute: ['/Mixer/Mix3/Adat5_mute'],
            self.mix3adat6_mute: ['/Mixer/Mix3/Adat6_mute'],
            self.mix3adat7_mute: ['/Mixer/Mix3/Adat7_mute'],
            self.mix3adat8_mute: ['/Mixer/Mix3/Adat8_mute'],
            self.mix3adat1_solo: ['/Mixer/Mix3/Adat1_solo'],
            self.mix3adat2_solo: ['/Mixer/Mix3/Adat2_solo'],
            self.mix3adat3_solo: ['/Mixer/Mix3/Adat3_solo'],
            self.mix3adat4_solo: ['/Mixer/Mix3/Adat4_solo'],
            self.mix3adat5_solo: ['/Mixer/Mix3/Adat5_solo'],
            self.mix3adat6_solo: ['/Mixer/Mix3/Adat6_solo'],
            self.mix3adat7_solo: ['/Mixer/Mix3/Adat7_solo'],
            self.mix3adat8_solo: ['/Mixer/Mix3/Adat8_solo'],
            self.mix3aes1_mute:  ['/Mixer/Mix3/Aes1_mute'],
            self.mix3aes2_mute:  ['/Mixer/Mix3/Aes2_mute'],
            self.mix3aes1_solo:  ['/Mixer/Mix3/Aes1_solo'],
            self.mix3aes2_solo:  ['/Mixer/Mix3/Aes2_solo'],
            self.mix3spdif1_mute: ['/Mixer/Mix3/Spdif1_mute'],
            self.mix3spdif2_mute: ['/Mixer/Mix3/Spdif2_mute'],
            self.mix3spdif1_solo: ['/Mixer/Mix3/Spdif1_solo'],
            self.mix3spdif2_solo: ['/Mixer/Mix3/Spdif2_solo'],
            self.mix3_mute:      ['/Mixer/Mix3/Mix_mute'],

            self.mix4ana1_mute:  ['/Mixer/Mix4/Ana1_mute'],
            self.mix4ana2_mute:  ['/Mixer/Mix4/Ana2_mute'],
            self.mix4ana3_mute:  ['/Mixer/Mix4/Ana3_mute'],
            self.mix4ana4_mute:  ['/Mixer/Mix4/Ana4_mute'],
            self.mix4ana5_mute:  ['/Mixer/Mix4/Ana5_mute'],
            self.mix4ana6_mute:  ['/Mixer/Mix4/Ana6_mute'],
            self.mix4ana7_mute:  ['/Mixer/Mix4/Ana7_mute'],
            self.mix4ana8_mute:  ['/Mixer/Mix4/Ana8_mute'],
            self.mix4ana1_solo:  ['/Mixer/Mix4/Ana1_solo'],
            self.mix4ana2_solo:  ['/Mixer/Mix4/Ana2_solo'],
            self.mix4ana3_solo:  ['/Mixer/Mix4/Ana3_solo'],
            self.mix4ana4_solo:  ['/Mixer/Mix4/Ana4_solo'],
            self.mix4ana5_solo:  ['/Mixer/Mix4/Ana5_solo'],
            self.mix4ana6_solo:  ['/Mixer/Mix4/Ana6_solo'],
            self.mix4ana7_solo:  ['/Mixer/Mix4/Ana7_solo'],
            self.mix4ana8_solo:  ['/Mixer/Mix4/Ana8_solo'],
            self.mix4adat1_mute: ['/Mixer/Mix4/Adat1_mute'],
            self.mix4adat2_mute: ['/Mixer/Mix4/Adat2_mute'],
            self.mix4adat3_mute: ['/Mixer/Mix4/Adat3_mute'],
            self.mix4adat4_mute: ['/Mixer/Mix4/Adat4_mute'],
            self.mix4adat5_mute: ['/Mixer/Mix4/Adat5_mute'],
            self.mix4adat6_mute: ['/Mixer/Mix4/Adat6_mute'],
            self.mix4adat7_mute: ['/Mixer/Mix4/Adat7_mute'],
            self.mix4adat8_mute: ['/Mixer/Mix4/Adat8_mute'],
            self.mix4adat1_solo: ['/Mixer/Mix4/Adat1_solo'],
            self.mix4adat2_solo: ['/Mixer/Mix4/Adat2_solo'],
            self.mix4adat3_solo: ['/Mixer/Mix4/Adat3_solo'],
            self.mix4adat4_solo: ['/Mixer/Mix4/Adat4_solo'],
            self.mix4adat5_solo: ['/Mixer/Mix4/Adat5_solo'],
            self.mix4adat6_solo: ['/Mixer/Mix4/Adat6_solo'],
            self.mix4adat7_solo: ['/Mixer/Mix4/Adat7_solo'],
            self.mix4adat8_solo: ['/Mixer/Mix4/Adat8_solo'],
            self.mix4aes1_mute:  ['/Mixer/Mix4/Aes1_mute'],
            self.mix4aes2_mute:  ['/Mixer/Mix4/Aes2_mute'],
            self.mix4aes1_solo:  ['/Mixer/Mix4/Aes1_solo'],
            self.mix4aes2_solo:  ['/Mixer/Mix4/Aes2_solo'],
            self.mix4spdif1_mute: ['/Mixer/Mix4/Spdif1_mute'],
            self.mix4spdif2_mute: ['/Mixer/Mix4/Spdif2_mute'],
            self.mix4spdif1_solo: ['/Mixer/Mix4/Spdif1_solo'],
            self.mix4spdif2_solo: ['/Mixer/Mix4/Spdif2_solo'],
            self.mix4_mute:      ['/Mixer/Mix4/Mix_mute'],

            self.ana1_pad:       ['/Mixer/Control/Ana1_pad'],
            self.ana2_pad:       ['/Mixer/Control/Ana2_pad'],
            self.ana3_pad:       ['/Mixer/Control/Ana3_pad'],
            self.ana4_pad:       ['/Mixer/Control/Ana4_pad'],
            self.ana5_level:     ['/Mixer/Control/Ana5_level'],
            self.ana6_level:     ['/Mixer/Control/Ana6_level'],
            self.ana7_level:     ['/Mixer/Control/Ana7_level'],
            self.ana8_level:     ['/Mixer/Control/Ana8_level'],
            self.ana5_boost:     ['/Mixer/Control/Ana5_boost'],
            self.ana6_boost:     ['/Mixer/Control/Ana6_boost'],
            self.ana7_boost:     ['/Mixer/Control/Ana7_boost'],
            self.ana8_boost:     ['/Mixer/Control/Ana8_boost'],
        }

        # Ultimately these may be rolled into the BinarySwitches controls,
        # but since they aren't implemented and therefore need to be
        # disabled it's easier to keep them separate for the moment.
        self.PairSwitches={
            self.mix1ana1_2_pair:   ['Mixer/Mix1/Ana1_2_pair'],
            self.mix1ana3_4_pair:   ['Mixer/Mix1/Ana3_4_pair'],
            self.mix1ana5_6_pair:   ['Mixer/Mix1/Ana5_6_pair'],
            self.mix1ana7_8_pair:   ['Mixer/Mix1/Ana7_8_pair'],
            self.mix1aes1_2_pair:   ['Mixer/Mix1/Aes1_2_pair'],
            self.mix1adat1_2_pair:  ['Mixer/Mix1/Adat1_2_pair'],
            self.mix1adat3_4_pair:  ['Mixer/Mix1/Adat3_4_pair'],
            self.mix1adat5_6_pair:  ['Mixer/Mix1/Adat5_6_pair'],
            self.mix1adat7_8_pair:  ['Mixer/Mix1/Adat7_8_pair'],
            self.mix1spdif1_2_pair: ['Mixer/Mix1/Spdif1_2_pair'],

            self.mix2ana1_2_pair:   ['Mixer/Mix2/Ana1_2_pair'],
            self.mix2ana3_4_pair:   ['Mixer/Mix2/Ana3_4_pair'],
            self.mix2ana5_6_pair:   ['Mixer/Mix2/Ana5_6_pair'],
            self.mix2ana7_8_pair:   ['Mixer/Mix2/Ana7_8_pair'],
            self.mix2aes1_2_pair:   ['Mixer/Mix2/Aes1_2_pair'],
            self.mix2adat1_2_pair:  ['Mixer/Mix2/Adat1_2_pair'],
            self.mix2adat3_4_pair:  ['Mixer/Mix2/Adat3_4_pair'],
            self.mix2adat5_6_pair:  ['Mixer/Mix2/Adat5_6_pair'],
            self.mix2adat7_8_pair:  ['Mixer/Mix2/Adat7_8_pair'],
            self.mix2spdif1_2_pair: ['Mixer/Mix2/Spdif1_2_pair'],

            self.mix3ana1_2_pair:   ['Mixer/Mix3/Ana1_2_pair'],
            self.mix3ana3_4_pair:   ['Mixer/Mix3/Ana3_4_pair'],
            self.mix3ana5_6_pair:   ['Mixer/Mix3/Ana5_6_pair'],
            self.mix3ana7_8_pair:   ['Mixer/Mix3/Ana7_8_pair'],
            self.mix3aes1_2_pair:   ['Mixer/Mix3/Aes1_2_pair'],
            self.mix3adat1_2_pair:  ['Mixer/Mix3/Adat1_2_pair'],
            self.mix3adat3_4_pair:  ['Mixer/Mix3/Adat3_4_pair'],
            self.mix3adat5_6_pair:  ['Mixer/Mix3/Adat5_6_pair'],
            self.mix3adat7_8_pair:  ['Mixer/Mix3/Adat7_8_pair'],
            self.mix3spdif1_2_pair: ['Mixer/Mix3/Spdif1_2_pair'],

            self.mix4ana1_2_pair:   ['Mixer/Mix4/Ana1_2_pair'],
            self.mix4ana3_4_pair:   ['Mixer/Mix4/Ana3_4_pair'],
            self.mix4ana5_6_pair:   ['Mixer/Mix4/Ana5_6_pair'],
            self.mix4ana7_8_pair:   ['Mixer/Mix4/Ana7_8_pair'],
            self.mix4aes1_2_pair:   ['Mixer/Mix4/Aes1_2_pair'],
            self.mix4adat1_2_pair:  ['Mixer/Mix4/Adat1_2_pair'],
            self.mix4adat3_4_pair:  ['Mixer/Mix4/Adat3_4_pair'],
            self.mix4adat5_6_pair:  ['Mixer/Mix4/Adat5_6_pair'],
            self.mix4adat7_8_pair:  ['Mixer/Mix4/Adat7_8_pair'],
            self.mix4spdif1_2_pair: ['Mixer/Mix4/Spdif1_2_pair'],
        }

        self.MixDests={
            self.mix1_dest:      ['/Mixer/Mix1/Mix_dest'],
            self.mix2_dest:      ['/Mixer/Mix2/Mix_dest'],
            self.mix3_dest:      ['/Mixer/Mix3/Mix_dest'],
            self.mix4_dest:      ['/Mixer/Mix4/Mix_dest'],

            self.phones_src:	 ['/Mixer/Control/Phones_src'],

            self.optical_in_mode:   ['/Mixer/Control/OpticalIn_mode'],
            self.optical_out_mode:  ['/Mixer/Control/OpticalOut_mode'],
        }

        self.SelectorControls={

        }

        # Other mixer variables
        self.is_streaming = 0
        self.sample_rate = 0

    def initValues(self):
        # Is the device streaming?
        self.is_streaming = self.hw.getDiscrete('/Mixer/Info/IsStreaming')
        print "device streaming flag: %d" % (self.is_streaming)

        # Retrieve other device settings as needed
        self.sample_rate = self.hw.getDiscrete('/Mixer/Info/SampleRate')
        print "device sample rate: %d" % (self.sample_rate)
        self.has_mic_inputs = self.hw.getDiscrete('/Mixer/Info/HasMicInputs')
        print "device has mic inputs: %d" % (self.has_mic_inputs)
        self.has_aesebu_inputs = self.hw.getDiscrete('/Mixer/Info/HasAESEBUInputs')
        print "device has AES/EBU inputs: %d" % (self.has_aesebu_inputs)
        self.has_spdif_inputs = self.hw.getDiscrete('/Mixer/Info/HasSPDIFInputs')
        print "device has SPDIF inputs: %d" % (self.has_spdif_inputs)
        self.has_optical_spdif = self.hw.getDiscrete('/Mixer/Info/HasOpticalSPDIF')
        print "device has optical SPDIF: %d" % (self.has_optical_spdif)

        # Customise the UI based on device options retrieved
        if (self.has_mic_inputs):
            # Mic input controls displace AES/EBU since no current device
            # has both.
            self.mix1_aes_group.setTitle("Mic inputs")
            # FIXME: when implmented, will mic channels just reuse the AES/EBU
            # dbus path?  If not we'll have to reset the respective values in
            # the control arrays (self.ChannelFaders etc).
        else:
            if (not(self.has_aesebu_inputs)):
                self.mix1_aes_group.setEnabled(False)
        if (not(self.has_spdif_inputs)):
            self.mix1_spdif_group.setEnabled(False)

        # Some devices don't have the option of selecting an optical SPDIF
        # mode.
        if (not(self.has_optical_spdif)):
            self.optical_in_mode.removeItem(2)
            self.optical_out_mode.removeItem(2)

        # Some controls must be disabled if the device is streaming
        if (self.is_streaming):
            print "Disabling controls which require inactive streaming"
            self.optical_in_mode.setEnabled(False)
            self.optical_out_mode.setEnabled(False)

        # Some channels aren't available at higher sampling rates
        if (self.sample_rate > 96000):
            print "Disabling controls not present above 96 kHz"
            self.mix1_adat_group.setEnabled(False)
            self.mix1_aes_group.setEnabled(False)
            self.mix1_spdif_group.setEnabled(False)
        if (self.sample_rate > 48000):
            print "Disabling controls not present above 48 kHz"
            self.mix1_adat58_group.setEnabled(False)

        # Now fetch the current values into the respective controls.  Don't
        # bother fetching controls which are disabled.
        for ctrl, info in self.ChannelFaders.iteritems():
            if (not(ctrl.isEnabled())):
                continue
            vol = 128-self.hw.getDiscrete(info[0])
            print "%s channel fader is %d" % (info[0] , vol)
            ctrl.setValue(vol)
            QObject.connect(ctrl, SIGNAL('valueChanged(int)'), self.updateFader)

        for ctrl, info in self.Controls.iteritems():
            if (not(ctrl.isEnabled())):
                continue
            pan = self.hw.getDiscrete(info[0])
            print "%s control is %d" % (info[0] , pan)
            ctrl.setValue(pan)
            QObject.connect(ctrl, SIGNAL('valueChanged(int)'), self.updateControl)

        # Disable the channel pair controls since they aren't yet implemented
        for ctrl, info in self.PairSwitches.iteritems():
            print "%s control is not implemented yet: disabling" % (info[0])
            ctrl.setEnabled(False)

        for ctrl, info in self.BinarySwitches.iteritems():
            if (not(ctrl.isEnabled())):
                continue
            val = self.hw.getDiscrete(info[0])
            print "%s switch is %d" % (info[0] , val)
            ctrl.setChecked(val)
            QObject.connect(ctrl, SIGNAL('toggled(bool)'), self.updateBinarySwitch)

        for ctrl, info in self.MixDests.iteritems():
            if (not(ctrl.isEnabled())):
                continue
            dest = self.hw.getDiscrete(info[0])
            print "%s mix destination is %d" % (info[0] , dest)
            ctrl.setCurrentItem(dest)
            QObject.connect(ctrl, SIGNAL('activated(int)'), self.updateMixDest)

        for name, ctrl in self.SelectorControls.iteritems():
            state = self.hw.getDiscrete(ctrl[0])
            print "%s state is %d" % (name , state)
            ctrl[1].setCurrentItem(state)    

        # FIXME: If optical mode is not ADAT, disable ADAT controls here. 
        # It can't be done earlier because we need the current values of the
        # ADAT channel controls in case the user goes ahead and enables the
        # ADAT optical mode.
