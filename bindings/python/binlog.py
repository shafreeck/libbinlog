#coding:utf8
from ctypes import *
import time
class Cell(Structure):
	"""A cell is an item of one row"""
	_fields_=[("ctype",c_int),("mtype",c_int),("length",c_int),("value",c_void_p)]
	INT32 = 0
	INT8 = 1
	INT16 = 2
	INT64 = 3
	UINT32 = 4
	UINT64 = 5
	FLOAT = 6 
	DOUBLE = 7
	STRING = 8
	BINARY = 9

	def __str__(self):
		# if self.value is NULL
		if not self.value:
			return 'nul'
		if self.ctype == Cell.INT32:
			return str(cast(self.value,POINTER(c_int32))[0])
		elif self.ctype == Cell.INT8:
			return str(cast(self.value,POINTER(c_int8))[0])
		elif self.ctype == Cell.INT16:
			return str(cast(self.value,POINTER(c_int16))[0])
		elif self.ctype == Cell.UINT32:
			return str(cast(self.value,POINTER(c_uint32))[0])
		elif self.ctype == Cell.UINT64:
			return str(cast(self.value,POINTER(c_uint64))[0])
		elif self.ctype == Cell.INT64:
			return str(cast(self.value,POINTER(c_int64))[0])
		elif self.ctype == Cell.FLOAT:
			return str(cast(self.value,POINTER(c_float))[0])
		elif self.ctype == Cell.DOUBLE:
			return str(cast(self.value,POINTER(c_double))[0])
		# In fact , mysql type binary correspond to MYSQL_TYPE_STRING(Cell.STRING) here
		elif self.ctype == Cell.BINARY or self.ctype == Cell.STRING: 
			s = ''
			p = cast(self.value,POINTER(c_char))
			for i in xrange(self.length):
				s += p[i]
			return s
		return '' #Return an empty string if no type match self.ctype
class BinlogRow(Structure):
	_fields_=[("nfields",c_int),("type",c_int),("created",c_int),("row",POINTER(Cell)),("rowOld",POINTER(Cell))]
class TableEv(Structure):
	_fields_=[("ts",c_uint32),("serverid",c_uint32),("nextpos",c_uint32),("evlen",c_uint32),
			("dbname",c_char*256),("tblname",c_char*256),("nfields",c_int),("tableid",c_uint64),
			("types",POINTER(c_uint8)),("canbenull",POINTER(c_uint8)),("meta",POINTER(c_uint16))]
class BinlogFde(Structure):
	_fields_ = [("blver",c_uint32),("hdrlen",c_uint8),("masterid",c_uint32),
			("server",c_char*50),("posthdr",c_char*29),("errstr",c_char*256),
			("table",TableEv)]

class MySQL(Structure):
	_fields_=[("fd",c_int),
			  ("scapacity",c_uint32),
			  ("ccapacity",c_uint32),
			  ("threadid",c_uint32),
			  ("maxpkt",c_uint32),
			  ("status",c_uint16),
			  ("charset",c_uint8),
			  ("pnum",c_uint8),
			  ("errstr",c_char*256)
			]

# C FILE handler
class FILE(Structure):
	pass
c_FILE_p = POINTER(FILE)

class DataSource(Structure):
	pass
class Driver(Union) :
	class Net(Structure):
		_fields_ = [("host",c_char_p),
					("port",c_int),
					("user",c_char_p),
					("passwd",c_char_p),
					("mysql",MySQL),
					("buf",c_char*1024*4),
					("newbuf",c_char_p)
				]
	class File(Structure):
		_fields_ = [("fp",c_FILE_p),
					("path",c_char_p),
					("buf",c_char*1024*4),
					("newbuf",c_char_p)
				]
	_fields_ = [("net",Net),("file",File)]

DataSource._fields_=[
		("connect",CFUNCTYPE(c_int,POINTER(DataSource))),
		("close",CFUNCTYPE(c_int,POINTER(DataSource))),
		("getEvent",CFUNCTYPE(c_char_p,POINTER(DataSource))),
		("freeEvent",CFUNCTYPE(None,POINTER(DataSource))),
		("url",c_char_p),
		("logfile",c_char*256),
		("position",c_uint32),
		("index",c_uint32),
		("serverid",c_int),
		("driver",Driver),
		("errstr",c_char*256)
		]

class BinlogClient(Structure):
	_fields_=[("_rows",POINTER(BinlogRow)),("_lenRows",c_int),("_cur",c_int),("_startidx",c_int),
			("binlog",BinlogFde),("dataSource",POINTER(DataSource)),
			("err",c_int),("errstr",c_char*256)]

class BinlogError(Exception):
	pass
class ConnectionError(BinlogError):
	pass
class FormatError(BinlogError):
	pass

class Binlog:
	#Event types
	BL_UNKNOWN_EVENT= 0
	BL_ROTATE_EVENT= 4    
	BL_FORMAT_DESCRIPTION_EVENT= 15
	BL_TABLE_MAP_EVENT = 19 
	BL_WRITE_ROWS_EVENT = 23 
	BL_UPDATE_ROWS_EVENT = 24 
	BL_DELETE_ROWS_EVENT = 25 
	BL_ENUM_END_EVENT = 28
	def __init__(self,url,serverid):
		self.bl = CDLL("libbinlog.so")
		self.connectDataSource = self.bl.connectDataSource
		self.connectDataSource.argtypes = [c_char_p,c_ulong,c_int,c_int]
		self.connectDataSource.restype = POINTER(BinlogClient)

		self.fetchOne = self.bl.fetchOne
		self.fetchOne.argtypes = [POINTER(BinlogClient)]
		self.fetchOne.restype = POINTER(BinlogRow)

		self.freeBinlogRow = self.bl.freeBinlogRow
		self.freeBinlogRow.argtypes = [POINTER(BinlogRow)]

		self.freeBinlogClient = self.bl.freeBinlogClient
		self.freeBinlogClient.argtypes = [POINTER(BinlogClient)]

		self.url = url
		self.serverid = serverid
	
	
	def open(self,pos,idx):
		self.cli = self.connectDataSource(self.url,pos,idx,self.serverid)[0]
		if self.cli.err!=0:
			raise ConnectionError("Connection error:%s"%self.cli.errstr)
		return self

	def fetch(self):
		"""Get one row from binlog"""
		prow = self.fetchOne(pointer(self.cli)) # return BinlogRow pointer
		if not bool(prow): # NULL pointer have a False boolean value
			if self.cli.err:
				raise BinlogError(self.cli.errstr)
			return None
		row = prow[0]
		entry = {}
		entry['db'] = self.cli.binlog.table.dbname
		entry['tbl'] = self.cli.binlog.table.tblname
		entry['ts'] = row.created
		entry['nfields'] = row.nfields
		ds = self.cli.dataSource.contents
		entry['pos'] ='%s/%s/%d'%(ds.logfile,ds.position,ds.index)
		if row.type == self.BL_WRITE_ROWS_EVENT:
			entry['act'] = 'insert'
			entry['row']={'new':[]}
		elif row.type == self.BL_UPDATE_ROWS_EVENT:
			entry['act'] = 'update'
			entry['row']={'new':[],'old':[]}
		elif row.type == self.BL_DELETE_ROWS_EVENT:
			entry['act'] = 'delete'
			entry['row']={'old':[]}
		else:
			entry['act'] = None

		if row.type == self.BL_UPDATE_ROWS_EVENT or row.type == self.BL_WRITE_ROWS_EVENT:
			for i in xrange(row.nfields):
				entry['row']['new'].append(str(row.row[i]))
		elif row.type == self.BL_DELETE_ROWS_EVENT:
			for i in xrange(row.nfields):
				entry['row']['old'].append(str(row.row[i]))
		if row.type == self.BL_UPDATE_ROWS_EVENT:
			for i in xrange(row.nfields):
				entry['row']['old'].append(str(row.rowOld[i]))
		self.freeBinlogRow(pointer(row));

		return entry 
	def close(self):
		self.freeBinlogClient(pointer(self.cli))

	def __iter__(self):  #add iteratable to binlog object
		while True:
			row = self.fetch()
			if not row:
				raise StopIteration()
			yield row
	
	def __enter__(self): #To support with statement
		return  self
	def __exit__(self,type,value,traceback):
		self.close()

def openbinlog(url,serverid,position=4,index=0):
	binlog = Binlog(url,serverid)
	return binlog.open(position,index)

def closebinlog(binlog):
	binlog.close()


if __name__ == '__main__':
	url = 'mysql://mytrigger:1q2w3e@10.73.11.211:4880/mysql-bin.000001'
#	url = 'mysql://mytrigger:1q2w3e@127.0.0.1:3306/mysql-bin.000001'
	with openbinlog(url,10,4,0) as binlog:
		for row in binlog:
			now = time.time()
			print now, row
	
