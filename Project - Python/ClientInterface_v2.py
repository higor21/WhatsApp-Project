# -*- coding: utf-8 -*-

# Form implementation generated from reading ui file 'ClientInterface.ui'
#
# Created by: PyQt4 UI code generator 4.12.1
#
# WARNING! All changes made in this file will be lost!

from socket import * 
from threading import Thread
import time, pickle
from classes import *
from queue import *
from os import system
from PyQt4 import QtCore, QtGui

ip_server = '127.0.0.1' # IP of server to connect
serverPort = 12000 # port to connect
clientSocket = socket(AF_INET, SOCK_STREAM)
clientSocket.connect((ip_server,serverPort))

# fila de mensagens a ser respondidas
listMessages = Queue()

# lista de usuários
listUserNames = []

cmd = False
ask = Message()
my_nick = ''
answer = ''
isPrivate = False

ui = object

try:
	_fromUtf8 = QtCore.QString.fromUtf8
except AttributeError:
	def _fromUtf8(s):
		return s

try:
	_encoding = QtGui.QApplication.UnicodeUTF8
	def _translate(context, text, disambig):
		return QtGui.QApplication.translate(context, text, disambig, _encoding)
except AttributeError:
	def _translate(context, text, disambig):
		return QtGui.QApplication.translate(context, text, disambig)

class Ui_Dialog(object):
	def setupUi(self, Dialog):
		Dialog.setObjectName(_fromUtf8("ZapApp"))
		Dialog.resize(377, 389)
		self.pushButton = QtGui.QPushButton(Dialog)
		self.pushButton.setGeometry(QtCore.QRect(310, 360, 61, 25))
		self.pushButton.setObjectName(_fromUtf8("pushButton"))
		self.textEdit = QtGui.QTextEdit(Dialog)
		self.textEdit.setGeometry(QtCore.QRect(30, 260, 341, 91))
		self.textEdit.setObjectName(_fromUtf8("textEdit"))
		self.label = QtGui.QLabel(Dialog)
		self.label.setGeometry(QtCore.QRect(10, 20, 67, 17))
		self.label.setObjectName(_fromUtf8("label"))
		self.label_2 = QtGui.QLabel(Dialog)
		self.label_2.setGeometry(QtCore.QRect(10, 230, 151, 17))
		self.label_2.setObjectName(_fromUtf8("label_2"))
		self.textEdit_2 = QtGui.QTextEdit(Dialog)
		self.textEdit_2.setGeometry(QtCore.QRect(30, 40, 341, 181))
		self.textEdit_2.setObjectName(_fromUtf8("textEdit_2"))
		self.pushButton_2 = QtGui.QPushButton(Dialog)
		self.pushButton_2.setGeometry(QtCore.QRect(280, 10, 89, 25))
		self.pushButton_2.setObjectName(_fromUtf8("pushButton_2"))
		self.pushButton_3 = QtGui.QPushButton(Dialog)
		self.pushButton_3.setGeometry(QtCore.QRect(280, 230, 89, 25))
		self.pushButton_3.setObjectName(_fromUtf8("pushButton_3"))
		
		# minhas alterações
		self.pushButton_3.clicked.connect(lambda: self.clear('txt_in'))
		self.pushButton_2.clicked.connect(lambda: self.clear('txt_out'))
		self.pushButton.clicked.connect(lambda: self.click())
		self.textEdit_2.setReadOnly(True)
		self.isClicked = True

		
		self.retranslateUi(Dialog)
		QtCore.QMetaObject.connectSlotsByName(Dialog)

	def retranslateUi(self, Dialog):
		Dialog.setWindowTitle(_translate("ZapApp", "ZapApp", None))
		self.pushButton.setText(_translate("ZapApp", "enviar", None))
		self.label.setText(_translate("ZapApp", "Servidor:", None))
		self.label_2.setText(_translate("ZapApp", "Mensagem/Comando:", None))
		self.pushButton_2.setText(_translate("ZapApp", "Limpar", None))
		self.pushButton_3.setText(_translate("ZapApp", "Limpar", None))
	
	def show_msg(self, msg, Dialog, type_ = ''):
		line = '================================'

		# retorna a string sem espaços nem '\n's no final
		if msg not in ['','\n',' ']:
			while msg[-1] in ['\n', ' ']:
				msg = msg[:-1]
		if type_ != '':
			msg = '\n' + type_ + ' ' + line + '\n' + msg + '\n' + (len(type_)-1)*'=' + line
		else:
			msg = '\n' + msg
		self.textEdit_2.append(msg)

	def clear(self, txt_type = 'txt_out'):
		if txt_type == 'txt_out':
			self.textEdit_2.clear()
		else :
			self.textEdit.clear()

	def get_input(self):
		self.isClicked = False
		while (not self.isClicked):
			QtGui.qApp.processEvents()
			time.sleep(0.01)
		self.isClicked = False

		# retorna a string sem espaços nem '\n's no final
		s = self.textEdit.toPlainText()
		if s not in ['','\n',' ']:
			while s[-1] in ['\n', ' ']:
				s = s[:-1]
		return s
	
	def click(self):
		self.isClicked = True

def getMessage():
	global listUserNames
	global ui
	while True:
		bitStream = clientSocket.recv(1024)
		if answer != 'sair()':
			message = Message(bitstream = bitStream)

			# print('-> ' + str(ask.command) + ' : ' + ask.nickname)
			
			if message.command == cmd_.ATUALIZAR and message.nickname not in listUserNames:
				listUserNames.append(message.nickname)

			if message.command == cmd_.LISTA_USERS:
				listUserNames = list(message.msg.split(';'))

			else:
				if message.command not in [cmd_.MOSTRAR, cmd_.ATUALIZAR]:
					listMessages.put(message)
				else:
					ui.show_msg('\n' + (message.nickname.replace(' ','') + ' escreveu: ' if message.nickname.replace(' ','') != my_nick.replace(' ','') else '') + str(message.msg) + '\n', Dialog)
					#print('\n' + (message.nickname.replace(' ','') + ' escreveu: ' if message.nickname.replace(' ','') != my_nick.replace(' ','') else '') + str(message.msg) + '\n')
		else :
			break

def printM():
	global cmd, ask, ui
	while True: 
		if not listMessages.empty() and not cmd and answer != 'sair()':
			ask = listMessages.get()
			ui.show_msg(ask.msg, Dialog)
			cmd = True
		elif answer == 'sair()':
			break

if __name__ == "__main__":
	import sys
	app = QtGui.QApplication(sys.argv)
	Dialog = QtGui.QDialog()
	ui = Ui_Dialog()
	ui.setupUi(Dialog)
	Dialog.show()
	
	#start here!
	ui.show_msg('Client started!', Dialog)
	
	Thread(target=getMessage, args=()).start()
	Thread(target=printM, args=()).start()

	while True:
		cmd = False
		nick , msg, command = '------', '', cmd_.CMD_PADRAO
		command = int(cmd_.CMD_PADRAO)
		#answer = input('\n' + my_nick + (': ' if my_nick != '' else '')) # espera por um comando

		#ui.show_msg('\n' + my_nick + (': ' if my_nick != '' else ''), Dialog)
		answer = ui.get_input()
		ui.clear('txt_in')
		if cmd:
			while True:
				accept = False
				if ask.command == cmd_.ACESSAR : 
					if list(answer).count('|') != 1 or answer.startswith('|') or answer.endswith('|') or len(answer.split('|')[0]) > 6:
						#print('nickname ou senha inválidos!\nInforme-os novamente')
						ui.show_msg('nickname ou senha inválidos!\nInforme-os novamente', Dialog, 'erro')
					else :
						my_nick = answer.split('|')[0] # nickname do usuário
						msg = answer
						accept = True
				elif ask.command == cmd_.LOG_CAD :
					if answer.upper() not in ['C', 'L']:
						#print("Digite um caractere apenas, podendo ser c,C,l,L")
						ui.show_msg("Digite um caractere apenas, podendo ser c,C,l,L", Dialog, 'erro')
					else:
						accept = True
						msg = answer.upper()
				elif ask.command == cmd_.LOG_REG or ask.command == cmd_.REQUISITO:
					if answer.upper() not in ['Y', 'N']:
						#print("Digite um caractere apenas, podendo ser y,Y,n,N")
						ui.show_msg("Digite um caractere apenas, podendo ser y,Y,n,N", Dialog, 'erro')
					else:
						accept = True
						msg = answer.upper()
						nick = ask.nickname
						command = cmd_.RESPOSTA
				elif ask.command == cmd_.SAIR:
					accept = True

				if not accept:		
					#answer = input(ask.msg)
					ui.show_msg(ask.msg, Dialog, 'erro')
					answer = ui.get_input()
					ui.clear('txt_in')
				else : 
					break
		else :
			if answer in ['lista()','sair()']:
				nick = my_nick
				command = ( cmd_.LISTA if answer == 'lista()' else cmd_.SAIR )
				if answer == 'sair()':
					clientSocket.send(bytes(Message(clientSocket.getsockname()[0],ip_server,nick,command,msg)))
			elif (answer.startswith('privado(') and answer.endswith(')')) or (answer.startswith('nome(') and answer.endswith(')')):
				inicio_cmd = (True if answer[0] == 'p' else False)
				nick = answer[len('privado(' if inicio_cmd else 'nome('): len(answer) - 1] 
				nick = (nick + (6 - len(nick))*' ')
				if nick != my_nick:
					if ((nick in listUserNames) if inicio_cmd else (nick not in listUserNames)):
						my_nick = (my_nick if inicio_cmd else nick)
						command = (cmd_.PRIVADO if inicio_cmd else cmd_.CG_NOME)
						isPrivate = inicio_cmd
					else:
						#print('\n' + ('Não' if inicio_cmd else 'Já') + ' existe usuários com este nome')
						ui.show_msg('\n' + ('Não' if inicio_cmd else 'Já') + ' existe usuários com este nome', 'erro')
						continue
				else:
					#print('\n ' + nick + ' é seu nome, digite outro nome \n')
					ui.show_msg('\n ' + nick + ' é seu nome, digite outro nome \n', 'erro')
					continue
			else:
				msg = answer
				nick = my_nick
				command = cmd_.ENVIAR
		
		if ask.command == cmd_.SAIR or (command == cmd_.SAIR and not isPrivate):
			system('clear')
			#print('\n\nvocê optou por sair!')
			ui.show_msg('\n\nvocê optou por sair!', Dialog)
			break

		clientSocket.send(bytes(Message(clientSocket.getsockname()[0],ip_server,nick,command,msg)))
	Dialog.close()
	sys.exit(app.exec_())

