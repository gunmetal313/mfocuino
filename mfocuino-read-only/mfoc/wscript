#! /usr/bin/env python
# encoding: utf-8
# Christophe Duvernois, 2012

top = '.'
out = 'build'

import os, sys, platform

from waflib import Task, Options, Logs
from waflib.TaskGen import feature, after
from waflib.TaskGen import extension

@extension('.pde', '.ino')
def process_pde_ino(self, node):
	if 'cxx' in self.features:
		cppnode = node.change_ext('.cpp');
		self.create_task('ino', node, cppnode)
		self.source.append(cppnode)
		
class ino(Task.Task):
	color = 'GREEN'
	
	def run(self):
		pde = open(self.inputs[0].abspath(), "r");
		cpp = open(self.outputs[0].abspath(), 'w')
		
		cpp.write('#if defined(ARDUINO) && ARDUINO >= 100\n')
		cpp.write('#include "Arduino.h"\n')
		cpp.write('#else\n')
		cpp.write('#include "WProgram.h"\n')
		cpp.write('#endif\n')
		cpp.write('void setup();\n')
		cpp.write('void loop();\n')

		for l in pde:
			cpp.write(l)
			
		pde.close()
		cpp.close()
	
class eep(Task.Task):
	color = 'YELLOW'
	run_str = '${OBJ_COPY} -O ihex -j .eeprom --set-section-flags=.eeprom=alloc,load --no-change-warnings --change-section-lma .eeprom=0 ${SRC} ${TGT}'
	
class hex(Task.Task):
	color = 'YELLOW'
	#run_str = '${OBJ_COPY} -O ihex -R .eeprom ${SRC} ${TGT}'
	
	def run(self):
		env = self.env
		bld = self.generator.bld
		wd = bld.bldnode.abspath()

		def to_list(xx):
			if isinstance(xx, str): return [xx]
			return xx		
		
		lst = []
		lst.extend(to_list(env['OBJ_COPY']))
		lst.extend(['-O'])
		lst.extend(['ihex'])
		lst.extend(['-R'])
		lst.extend(['.eeprom'])
		lst.extend([self.inputs[0].abspath()])
		lst.extend([self.outputs[0].abspath()])
		lst = [x for x in lst if x]
		
		self.generator.bld.cmd_and_log(lst, cwd=wd, env=env.env or None, output=0, quiet=0)
		
		lst = []
		lst.extend(to_list(env['AVR_SIZE']))
		lst.extend([self.outputs[0].abspath()])
		lst = [x for x in lst if x]
		
		out = self.generator.bld.cmd_and_log(lst, cwd=wd, env=env.env or None, output=0, quiet=0)[0]
		size = int(out.splitlines()[1].split('\t')[1].strip())
		maxsize = int(env.ARDUINO['upload.maximum_size'])
		if(size <= maxsize):
			Logs.pprint('BLUE', 'Binary sketch size: %d bytes (of a %s byte maximum)' % (size, maxsize))
		else:
			Logs.pprint('RED', 'Binary sketch size: %d bytes (of a %s byte maximum)' % (size, maxsize))

@feature('cxxprogram', 'cprogram') 
@after('apply_link') 
def makeUploadArduinoProgram(self): 
	elfnode = self.link_task.outputs[0]
	eepnode = elfnode.change_ext(".eep")
	hexnode = elfnode.change_ext(".hex")
	self.create_task('eep', elfnode, eepnode)
	self.create_task('hex', elfnode, hexnode)

def getAvailableBoards(ctx):
	boardsfile = os.path.abspath(ctx.env.ARDUINO_PATH + '/hardware/arduino/boards.txt')
	try:
		brd = open(boardsfile, "r");
		boards = {}
		for l in brd:
			l = l.strip()
			if len(l) == 0 or l.startswith('#'):
				continue
			property = l.split('=')
			value = property[1]
			property = property[0].split('.', 1)
			board = property[0]
			if not board in boards:
				boards[board] = {}
			boards[board][property[1]] = value
		return boards
	except Exception, e:
		print e;

def checkArduinoIdeVersion(ctx):
	revisionfile = os.path.abspath(ctx.env.ARDUINO_PATH + '/revisions.txt')
	try:
		rev = open(revisionfile, "r");
		#read the first line (should be ARDUINO {version} - {date}
		line = rev.readline()
		if line.startswith('ARDUINO'):
			version = line.split(' ')[1].strip()
			ctx.msg('Checking arduino version', version)
			version = version.replace('.', '')
			ctx.env.ARDUINO_IDE_VERSION = version
			return
	except Exception, e:
		pass
	ctx.msg('Checking arduino version', 'not found', 'YELLOW')

def options(opt):
	opt.load('compiler_c')
	opt.load('compiler_cxx')
	opt.add_option('--path', action='store', default='', help='path of the arduino ide', dest='idepath')
	opt.add_option('--arduino', action='store', default='uno', help='Arduino Board (uno, mega, ...), default=uno', dest='board')

def boards(ctx):
	"""display available arduino boards """
	arduinos = getAvailableBoards(ctx)
	Logs.pprint('BLUE', "Arduino boards available :")
	for b in arduinos:
		Logs.pprint('NORMAL', "%s : %s" % (b.ljust(max(15, len(b))), arduinos[b]['name']))


def configure(conf):	
	searchpath = []
	relBinPath = ['hardware/tools/', 'hardware/tools/avr/bin/']
	relIncPath = ['hardware/arduino/cores/arduino/', 'hardware/arduino/variants/standard/']
	
	if os.path.exists(Options.options.idepath):
		conf.env.ARDUINO_PATH = os.path.abspath(Options.options.idepath)
	else:
		if platform.system() == "Windows":
			conf.fatal('you have to specify the arduino ide installation path with --path')
		else:
			#try package installation ... (will work on ubuntu ...)
			conf.env.ARDUINO_PATH = '/usr/share/arduino'
		
	for p in relBinPath:
		path = os.path.abspath(conf.env.ARDUINO_PATH + '/' + p)
		if os.path.exists(path):
			searchpath += [path]
		avrdudeconf = os.path.abspath(path + '/avrdude.conf')
		if os.path.exists(avrdudeconf):
			conf.env.AVRDUDECONF = avrdudeconf
	
	for p in relIncPath:
		path = os.path.abspath(conf.env.ARDUINO_PATH + '/' + p)
		if os.path.exists(path):
			conf.env.INCLUDES += [path]

	
	searchpath += os.environ['PATH'].split(os.pathsep)
	searchpath = filter(None, searchpath)

	conf.find_program('avr-gcc', var='CC', path_list=searchpath)
	conf.find_program('avr-g++', var='CXX', path_list=searchpath)
	conf.find_program('avr-ar', var='AR', path_list=searchpath)
	conf.find_program('avr-objcopy', var='OBJ_COPY', path_list=searchpath)
	conf.find_program('avr-size', var='AVR_SIZE', path_list=searchpath)
	conf.find_program('avrdude', var='AVRDUDE', path_list=searchpath)
	
	if conf.env.AVRDUDECONF:
		conf.msg('Checking for avrdude.conf', conf.env.AVRDUDECONF)
	else:
		conf.msg('Checking for avrdude.conf', 'not found', 'YELLOW')
	
	conf.load('compiler_c')
	conf.load('compiler_cxx')

	# arduino board ...
	boardsfile = os.path.abspath(conf.env.ARDUINO_PATH + '/hardware/arduino/boards.txt')
	if os.path.exists(boardsfile):
		arduinos = getAvailableBoards(conf)
	else:
		conf.fatal('Can\'t find boards description file : %s' % boardsfile)
	
	if not Options.options.board in arduinos:
		conf.fatal('Arduino ', 'Unknown board %s!' % Options.options.board)
	arduino = arduinos[Options.options.board]
	conf.env.ARDUINO = arduino
	
	checkArduinoIdeVersion(conf)
	conf.msg('Arduino ', '%s' % arduino['name'], 'BLUE')
	
	flags = ['-g', '-Os' , '-w' , '-Wall', '-std=c99', '-fno-exceptions', '-ffunction-sections' , '-fdata-sections', '-mmcu=%s' % conf.env.ARDUINO['build.mcu'], '-MMD']
	conf.env.CFLAGS = flags
	conf.env.CXXFLAGS = flags
	conf.env.DEFINES = ['F_CPU=%s' % conf.env.ARDUINO['build.f_cpu'], 'USB_VID=null', 'USB_PID=null']
	if conf.env.ARDUINO_IDE_VERSION:
		conf.env.DEFINES += ['ARDUINO=%s' % conf.env.ARDUINO_IDE_VERSION]
	conf.env.LINKFLAGS = ['-Wl,--gc-sections', '-mmcu=%s' % conf.env.ARDUINO['build.mcu']]
	
	conf.env.INCLUDES += ['libraries']
	conf.env.INCLUDES += ['%s/libraries' % conf.env.ARDUINO_PATH]
	
def buildArduinoCore(bld):
	#build arduino core library
	node = bld.root.ant_glob([
		'%s/hardware/arduino/cores/arduino/*.c' % bld.env.ARDUINO_PATH[1:],
		'%s/hardware/arduino/cores/arduino/*.cpp' % bld.env.ARDUINO_PATH[1:],
		#add extra libraries if needed ...
		])
	bld(features='c cxx cxxstlib', source=node, target='core', lib='m')      

def build(bld):
	#build arduino core library
	bld.add_pre_fun(buildArduinoCore)

	#build arduino project
	src = bld.path.ant_glob(['src/**/*.pde', 'src/**/*.ino', 'src/**/*.c', 'src/**/*.cpp', 'libraries/**/*.c', 'libraries/**/*.cpp'])
	bld(features = 'c cxx cxxprogram',
		includes = 'src',
		source = src, 
		target = 'mfocuino.elf',
		lib = 'm',
		use = 'core',
	)

def upload(ctx):
	"""Upload to arduino."""
	hexnode = ctx.bldnode.find_node('mfocuino.elf')
	if not hexnode:
		Options.commands = ['build', 'upload'] + Options.commands
		return

	import subprocess	
	cmd = [ctx.env.AVRDUDE]
	cmd += ['-C%s' % ctx.env.AVRDUDECONF]
	cmd += ['-v']
	cmd += ['-p%s' % ctx.env.ARDUINO['build.mcu']]
	cmd += ['-carduino']
	cmd += ['-P/dev/ttyACM0']
	cmd += ['-b%s' % ctx.env.ARDUINO['upload.speed']]
	cmd += ['-D']
	cmd += ['-Uflash:w:%s:i' % hexnode.abspath()]
	print " ".join(cmd)
	subprocess.call(cmd)
	
def monitor(ctx):
	"""This is meant to be an equivalent to the Arduino serial monitor."""
	import serial, select
	# Port may vary, so look for it:
	baseports = ['/dev/ttyUSB', '/dev/ttyACM']
	baud = 9600
	ser = None
	for baseport in baseports :
		if ser : break
		for i in xrange(0, 8) :
			try :
				port = baseport + str(i)
				ser = serial.Serial(port, baud, timeout=1)
				Logs.pprint('GREEN', "Opened %s" % port)
				break
			except :
				ser = None
				pass

	if not ser :
		ctx.fatal("Couldn't open a serial port")

	ser.flushInput()
	
	Logs.pprint('GREEN', 'Arduino Monitor :')
	
	try:
		while True :
			# Check whether the user has typed anything:
			inp, outp, err = select.select([sys.stdin, ser], [], [], .2)
			# Check for user input:
			if sys.stdin in inp :
				line = sys.stdin.readline()
				ser.write(line)
			# check for Arduino output:
			if ser in inp :
				line = ser.readline().strip()
				print line
	except KeyboardInterrupt :
		ser.close()
		Logs.pprint('GREEN', '\n%s closed' % port)
		return
	except Exception, e:
		ser.close()
		Logs.pprint('RED', e)


from waflib.Build import BuildContext
class BoardsList(BuildContext):
	cmd = 'boards'
	fun = 'boards'

class Upload(BuildContext):
	cmd = 'upload'
	fun = 'upload'
