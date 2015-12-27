#!/usr/bin/env python2
# -*- coding: utf-8 -*-
# ubuntu narodmonindicator v 0.1
# by sash0k <gav-sash0k@yandex.ru> 2015

import sys  # для языка

import os #для поиска изображения в файловой системе
reload(sys)  
sys.setdefaultencoding('utf8')


import signal # для обработки нажатия на меню
import json # для парсинга

from urllib2 import Request, urlopen, URLError # для запроса на сервер
from gi.repository import Gtk as gtk
from gi.repository import AppIndicator3 as appindicator
from gi.repository import Notify as notify # вывод всплывающего сообщения

APPINDICATOR_ID = 'myappindicator'

# структура API запроса для narodmon, в режиме отображения сенсоров указанного устройства
#{"cmd":"sensorsOnDevice","id":ID,"uuid":"UUID","api_key":"API_KEY","lang":"ru"}
data = {
    'cmd': 'sensorsOnDevice',
    'id': 'xxxxx', # Идертификатор вашего устройства
    'uuid': 'xxxxxxxxxxxxxxxxxxxxxxxxx', #уникальный ID приложения (md5 хеш)
    'api_key': 'xxxxxxxxxxxx', # API ключ сгенерированный для конкретного приложения на Narodmon.ru
    'lang': 'ru'
}
jsoninput = json.dumps(data)

def main():
    indicator = appindicator.Indicator.new(APPINDICATOR_ID, os.path.abspath('pressure_fat.svg'), appindicator.IndicatorCategory.SYSTEM_SERVICES)

    indicator.set_status(appindicator.IndicatorStatus.ACTIVE)
    indicator.set_label("Narodmon", "")
    indicator.set_menu(build_menu())
    notify.init(APPINDICATOR_ID)
    gtk.main()

def fetch_presure():
	request = Request('http://narodmon.ru/api', jsoninput)
	response = urlopen(request)
	presure_obj = json.loads(response.read())['sensors'][1]
	
	return presure_obj
	
def build_menu():
	menu = gtk.Menu()

	item_presure = gtk.MenuItem('Показать давление')
	item_presure.connect('activate',presure)
	menu.append(item_presure)

	item_separator = gtk.SeparatorMenuItem()
	menu.append(item_separator)

	item_quit = gtk.MenuItem('Выход')
	item_quit.connect('activate',quit)
	menu.append(item_quit)
	menu.show_all()
	return menu

def presure(_):
	pres_obj = fetch_presure()
	notify.Notification.new("<b>" + pres_obj['name'] +"</b>", str(pres_obj['value']) +" " + str(pres_obj['unit']),None).show()
	indicator.set_label("Narodmon", "")
def quit(source):
	notify.uninit()
	gtk.main_quit()

if __name__ == "__main__":
    signal.signal(signal.SIGINT, signal.SIG_DFL)
    main()