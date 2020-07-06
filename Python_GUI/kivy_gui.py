import random
from kivy.config import Config
Config.set('kivy', 'log_enable', 0)
Config.write()

from kivy.app import App
from kivy.uix.switch import Switch
from kivy.uix.button import Button
from kivy.uix.widget import Widget
from kivy.graphics import Color, Ellipse, Rectangle
from kivy.uix.gridlayout import GridLayout
from kivy.uix.relativelayout import RelativeLayout
from kivy.properties import NumericProperty
from kivy.clock import Clock
from kivy.config import Config

import zmq
import thread

DEBUG = False

DEFAULT_SENSOR_DATA = 930
POKED_SENSOR_DATA = 220
SENSOR_DATA = [DEFAULT_SENSOR_DATA, POKED_SENSOR_DATA]

DIGITAL_PIN_NUM = 53
POKE_NUM = 8

# Depreciated: Should better not have two copies of Pin to Poke Mapping
PortDigitalOutputLines = [28, 30, 32, 34, 36, 38, 40, 42]
PortPWMOutputLines = [9, 8, 7, 6, 5, 4, 3, 2]

digital_pin_data = [0] * (DIGITAL_PIN_NUM + 1)
analog_pin_data  = [DEFAULT_SENSOR_DATA] * POKE_NUM

outputPort = 8003
inputPort  = 8002

#LAYOUT_CONFIG = [0, 1, 0, 0, 0, 1, 1, 1, 0, 1, 1, 1, 1, 1]
LAYOUT_CONFIG = [0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 2, 0, 3, 0, 0]
POKE_CONFIG   = [0, 2, 0, 3, 1, 3, 0, 2, 0]
LED_SIZE = 10

# POKE_INDEXING = [0, 3, 2, 1, 7, 6, 8, 5, 4]
POKE_INDEXING = [4, 1, 6, 3, 0, 5, 2, 7]


context = zmq.Context()

receiver = context.socket(zmq.PULL)
receiver.connect("tcp://127.0.0.1:8002")

sender = context.socket(zmq.PUSH)
sender.bind('tcp://127.0.0.1:8003')

# Wait on socket for incoming digital pin data
def receiver_thread_exec():
    CNT = 0
    while True:
        idx, data = receiver.recv().split(':')
        if DEBUG and (int(idx)==DIGITAL_PIN_NUM):
            print CNT, digital_pin_data
            CNT += 1

        digital_pin_data[int(idx)] = int(data)

thread.start_new_thread(receiver_thread_exec, ())

# Send analog pin data to shared memory
def send_sensory_data(poke_idx):
    sender.send_string('%d:%3d' % (poke_idx, analog_pin_data[poke_idx]))

# Initilization
for i in range(0, POKE_NUM):
    send_sensory_data(i)

# Widget for GUI
class PlaceHolderWidget(Widget):
    pass

class ValveStateWidget(Widget):
    alpha_value = NumericProperty(1.0)
    def __init__(self, **kwargs):
        self.led_size = (LED_SIZE, LED_SIZE)
        super(ValveStateWidget, self).__init__(**kwargs)
        Clock.schedule_interval(self.update_color, 1.0 / 60.0)

    def update_color(self, dt):
        self.alpha_value = float(digital_pin_data[-1])

class PwmLedWidget(RelativeLayout):
    alpha_value = NumericProperty(1.0)

    def __init__(self, index, **kwargs):
        self.index = index
        self.led_pos = [x/2 - x/4 for x in self.size]
        self.led_pos[0] -= 13
        self.led_pos[1] -= 10

        self.led_size = (LED_SIZE, LED_SIZE)

        super(PwmLedWidget, self).__init__(**kwargs)
        Clock.schedule_interval(self.update_color, 1.0 / 60.0)

    def update_color(self, dt):
        pwm = digital_pin_data[PortPWMOutputLines[self.index]]
        if pwm == 0:
            self.alpha_value = 0
        else:
            pwm = float(pwm) / 255.0
            self.alpha_value = 0.6 + pwm * 0.4

class RewardLedWidget(RelativeLayout):
    alpha_value = NumericProperty(1.0)

    def __init__(self, index, **kwargs):
        self.index = index
        self.led_pos = [self.width/2, self.height/2]

        #self.led_pos[0] = self.led_pos[0] - self.size[0]/4 - 13
        #self.led_pos[1] = self.led_pos[0]

        self.led_size = (LED_SIZE, LED_SIZE)


        super(RewardLedWidget, self).__init__(**kwargs)
        Clock.schedule_interval(self.update_color, 1.0 / 60.0)

    def update_color(self, dt):
        pwm = digital_pin_data[PortPWMOutputLines[self.index]]
        if pwm == 0:
            self.alpha_value = 0
        else:
            pwm = float(pwm) / 255.0
            self.alpha_value = 0.6 + pwm * 0.4

class BinaryLedWidget(RelativeLayout):
    alpha_value = NumericProperty(1.0)

    def __init__(self, index, **kwargs):
        self.index = index
        self.led_pos = [x/2 for x in self.size]

        self.led_pos[0] = self.led_pos[0] - self.size[0]/4 - 13
        self.led_pos[1] = self.led_pos[0]

        self.led_size = (12, 12)


        super(BinaryLedWidget, self).__init__(**kwargs)
        Clock.schedule_interval(self.update_color, 1.0 / 60.0)

    def update_color(self, dt):
        self.alpha_value = float(digital_pin_data[PortDigitalOutputLines[self.index]])


class RewardPokeWidget(GridLayout):
    def __init__(self, index, **kwargs):
        super(RewardPokeWidget, self).__init__(**kwargs)

        self.index = index
        self.rows = 3
        self.cols = 3
        self.width = 85
        self.height = 85
        rew_poke_config = [0, 0, 0, 0, 1, 0, 0, 2, 0]

        for idx in rew_poke_config:
            if idx == 1:
                # switch = Switch()
                # switch.bind(active=self.switch_callback)
                # self.add_widget(switch)
                button = Button(text='Poke',pos=[self.width/2, self.height/2])
                button.bind(on_press=self.press_callback)
                button.bind(on_release=self.release_callback)
                self.add_widget(button)
            elif idx == 2:
                self.rewLED = RewardLedWidget(self.index)
                self.add_widget(self.rewLED)
            else:
                self.add_widget(PlaceHolderWidget())

    def switch_callback(self, instance, value):
        analog_pin_data[self.index] = SENSOR_DATA[value]
        send_sensory_data(self.index)

    def press_callback(self, instance):
        analog_pin_data[self.index] = POKED_SENSOR_DATA
        send_sensory_data(self.index)

    def release_callback(self, instance):
        analog_pin_data[self.index] = DEFAULT_SENSOR_DATA
        send_sensory_data(self.index)


class PokeWidget(GridLayout):
    def __init__(self, index, **kwargs):
        super(PokeWidget, self).__init__(**kwargs)

        self.index = index
        self.rows = 3
        self.cols = 3
        self.size = [80,80]

        for idx in POKE_CONFIG:
            if idx == 1:
                # switch = Switch()
                # switch.bind(active=self.switch_callback)
                # self.add_widget(switch)
                button = Button(text='Poke')
                button.bind(on_press=self.press_callback)
                button.bind(on_release=self.release_callback)
                self.add_widget(button)
            elif idx == 2:
                self.binaryLED = BinaryLedWidget(self.index)
                self.add_widget(self.binaryLED)
            elif idx == 3:
                self.pwmLED = PwmLedWidget(self.index)
                self.add_widget(self.pwmLED)
            else:
                self.add_widget(PlaceHolderWidget())

    def switch_callback(self, instance, value):
        analog_pin_data[self.index] = SENSOR_DATA[value]
        send_sensory_data(self.index)

    def press_callback(self, instance):
        analog_pin_data[self.index] = POKED_SENSOR_DATA
        send_sensory_data(self.index)

    def release_callback(self, instance):
        analog_pin_data[self.index] = DEFAULT_SENSOR_DATA
        send_sensory_data(self.index)


class PokeWall(GridLayout):
    def __init__(self, **kwargs):
        super(PokeWall, self).__init__(padding=[0, 0, 0, 0], **kwargs)

        #super(PokeWall, self).__init__(padding=[20, 20, 20, 20], **kwargs)
        self.rows = 4
        self.cols = 5

class BpodEmulator(App):
    def build(self):
        pokewall = PokeWall()
        poke_idx = 0
        for idx in LAYOUT_CONFIG:
            if idx == 1:
                poke = PokeWidget(index=POKE_INDEXING[poke_idx],width=60)
                pokewall.add_widget(poke)
                poke_idx = poke_idx + 1
            elif idx == 2:
                pokewall.add_widget(ValveStateWidget())
            elif idx == 3:
                poke = RewardPokeWidget(index=POKE_INDEXING[poke_idx])
                pokewall.add_widget(poke)
                poke_idx = poke_idx + 1
            else:
                pokewall.add_widget(PlaceHolderWidget())

        return pokewall

Config.set('graphics', 'width', '600')
Config.set('graphics', 'height', '370')
BpodEmulator().run()
