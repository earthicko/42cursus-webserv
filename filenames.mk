# -------------------------------- directory --------------------------------- #

DIR_SRCS 			= srcs/
DIR_OBJS 			= objs/

DIR_ASYNC			= async/
DIR_ASYNC_IO 		= $(DIR_ASYNC)IOProcessor/
DIR_ASYNCFILE		= $(DIR_ASYNC)FileIOHandler/
DIR_ASYNCLOGGER		= $(DIR_ASYNC)Logger/
DIR_CONFIG_PARSER 	= ConfigParser/
DIR_HTTP 			= HTTP/
DIR_CGI 			= CGI/
DIR_WEBSERVER		= WebServer/

DIR_TESTSRCS 		= test/testsrcs/
DIR_TESTOBJS 		= test/objs/

DIR_TESTCASE 		= test/testcase/

# -------------------- driver source (with main function) -------------------- #

DRIVERNAMES			= main \

DRIVERSRCS			= $(addprefix $(DIR_SRCS), $(addsuffix .cpp, $(DRIVERNAMES)))
DRIVEROBJS			= $(addprefix $(DIR_OBJS), $(addsuffix .o, $(DRIVERNAMES)))

TESTDRIVERNAMES		=	\
					test_asyncio_echo \
					test_asyncio_singleio \
					test_asynclogger \
					test_asyncfilereader \
					test_asyncfilewriter \
					test_configparser \
					test_http_request \
					test_http_response \
					test_http_server_constructor \
					test_bidimap \
					test_shared_ptr \

TESTDRIVERSRCS		= $(addprefix $(DIR_TESTSRCS), $(addsuffix .cpp, $(TESTDRIVERNAMES)))
TESTDRIVEROBJS		= $(addprefix $(DIR_TESTOBJS), $(addsuffix .o, $(TESTDRIVERNAMES)))
TESTDRIVERDEPS		= $(addprefix $(DIR_TESTOBJS), $(addsuffix .d, $(TESTDRIVERNAMES)))

# ---------------------- source (without main function) ---------------------- #

FILENAMES			= \
					utils/string \
					utils/file \
					utils/hash \
					Header/Header \
					$(DIR_ASYNC)generateErrorMsg \
					$(DIR_ASYNC_IO)IOProcessor \
					$(DIR_ASYNC_IO)SingleIOProcessor \
					$(DIR_ASYNC_IO)TCPIOProcessor \
					$(DIR_ASYNCFILE)FileIOHandler \
					$(DIR_ASYNCFILE)FileReader \
					$(DIR_ASYNCFILE)FileWriter \
					$(DIR_ASYNCLOGGER)Logger \
					$(DIR_ASYNCLOGGER)EndMarker \
					$(DIR_CONFIG_PARSER)ConfigDirective \
					$(DIR_CONFIG_PARSER)InvalidDirective \
					$(DIR_CONFIG_PARSER)parseConfig \
					$(DIR_CONFIG_PARSER)recursiveParser \
					$(DIR_CONFIG_PARSER)splitIntoTokens \
					$(DIR_HTTP)const_values \
					$(DIR_HTTP)mime_type \
					$(DIR_HTTP)error_pages \
					$(DIR_HTTP)ParsingFail \
					$(DIR_HTTP)Request/Request \
					$(DIR_HTTP)Request/RequestParse \
					$(DIR_HTTP)Request/RequestParseUtils \
					$(DIR_HTTP)Request/RequestConsume \
					$(DIR_HTTP)Response/Response \
					$(DIR_HTTP)Response/ResponseInit \
					$(DIR_HTTP)Response/ResponseSetter \
					$(DIR_HTTP)Server/RequestHandler/RequestHandler \
					$(DIR_HTTP)Server/RequestHandler/RequestGetHandler \
					$(DIR_HTTP)Server/RequestHandler/RequestHeadHandler \
					$(DIR_HTTP)Server/RequestHandler/RequestPostHandler \
					$(DIR_HTTP)Server/RequestHandler/RequestPutHandler \
					$(DIR_HTTP)Server/RequestHandler/RequestDeleteHandler \
					$(DIR_HTTP)Server/ErrorResponseHandler \
					$(DIR_HTTP)Server/Location \
					$(DIR_HTTP)Server/LocationParseDirective \
					$(DIR_HTTP)Server/Server \
					$(DIR_HTTP)Server/ServerMethods \
					$(DIR_HTTP)Server/ServerInterfaces \
					$(DIR_HTTP)Server/ServerParseDirective \
					$(DIR_HTTP)Server/ServerError \
					$(DIR_CGI)const_values \
					$(DIR_CGI)Request \
					$(DIR_CGI)Response \
					$(DIR_CGI)RequestHandler \
					$(DIR_CGI)RequestHandlerPipe \
					$(DIR_CGI)RequestHandlerVnode \
					$(DIR_WEBSERVER)WebServer \
					$(DIR_WEBSERVER)WebServerMethod \
					$(DIR_WEBSERVER)WebServerParseDirective \

SRCS				= $(addprefix $(DIR_SRCS), $(addsuffix .cpp, $(FILENAMES)))
OBJS				= $(addprefix $(DIR_OBJS), $(addsuffix .o, $(FILENAMES)))
DEPS				= $(addprefix $(DIR_OBJS), $(addsuffix .d, $(FILENAMES)))
