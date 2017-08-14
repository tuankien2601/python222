import e32
import os

_rfs = e32._def_rfs()
sep = '\\'

def abspath(path):
	return normpath(join('c:', path))

def basename(path):
	return split(path)[1]

def dirname(path):
	return split(path)[0]

def isdir(path):
	path = path.rstrip(sep) + sep
	try:
		ret = _rfs.IsDir(unicode(path))
	except OSError:
		return 0
	return ret

def isfile(path):
	path = path.rstrip(sep) + sep
	try:
		ret = not _rfs.IsDir(unicode(path))
	except OSError:
		return 0
	return ret

def join(a, *p):
	path = a.rstrip(sep)
	for x in p:
		if len(x)>1 and x[1] == ':':
			path = x
		elif x and x[0] == sep:
			if len(path) > 1 and path[1] == ':':
				path = path[0:2] + x
			else:
				path = x
		else:
			if len(path) == 2 and path[1] == ':':
# Portions Copyright (c) 2005 Nokia Corporation 
				path = path + x.rstrip(sep)
			elif x:
				path = path + sep + x.strip(sep)
	return path

def normpath(path):
	p = path
	while p.find(sep + sep) != -1: p = p.replace(sep + sep, sep)
	while p.find(sep + '.' + sep) != -1: p = p.replace(sep + '.' + sep, '')
	while p.find(sep + '..' + sep) != -1:
		p = p[:p[:p.rfind(sep + '..' + sep)].rfind(sep)] \
			+ p[p.rfind(sep + '..' + sep)+3:]
	if len(p) > 2 and len(p.rstrip(sep)) == 2:
		return p[:2]
	return p.rstrip(sep)
	
def split(path):
	p = path.rfind(sep)
	if p == -1:
		return ('', path)
	if len(path)>2 and path[1] == ':' and len(path.rstrip(sep)) == 2:
		return (path[0:2], path[3:].strip(sep))
	return (path[:p].rstrip(sep), path[p+1:])

def splitdrive(path):
	drive = ''
	tail = path
	if len(path)>1 and path[1]==':':
		drive = path[0:2]
		tail = path[2:]
	return (drive, tail)

def splitext(path):
	root = path
	ext = ''
	if path.rfind('.') != -1:
		if path.rfind('.') > path.rfind('\\'):
			root = path[:path.rfind('.')-1]
			ext = path[path.rfind('.'):]
	return (root, ext)

def walk(top, func, arg):
	try:
		names = os.listdir(top)
	except:
		return
	func(arg, top, names)
	exceptions = ('.', '..')
	for name in names:
		if name not in exceptions:
			name = join(top, name)
			if isdir(name):
				walk(name, func, arg)
