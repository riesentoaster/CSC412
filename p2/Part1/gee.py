
import re, sys

debug = False
dict = { }
tokens = [ ]



#  Expression class and its subclasses
class Expression( object ):
	def meaning(self, state):
		return None

	def __str__(self):
		return "" 
	
class BinaryExpr( Expression ):
	def __init__(self, op, left, right):
		self.op = op
		self.left = left
		self.right = right
		
	def meaning(self, state):
		operators = {
			'and': lambda a,b: a and b,
			'or': lambda a,b: a or b,
			'>': lambda a,b: a>b,
			'>=': lambda a,b: a>=b,
			'<': lambda a,b: a<b,
			'<=': lambda a,b: a<=b,
			'==': lambda a,b: a==b,
			'!=': lambda a,b: a!=b,
			'+': lambda a,b: a+b,
			'-': lambda a,b: a-b,
			'*': lambda a,b: a*b,
			'/': lambda a,b: a/b,
		}
		return operators.get(self.op)(self.left.meaning(state), self.right.meaning(state))
	
	def __str__(self):
		return str(self.op) + " " + str(self.left) + " " + str(self.right)

class Assign(Expression):
	def __init__(self, left, right):
		self.left = left
		self.right = right
		
	def meaning(self, state):
		return {**state, str(self.left):self.right.meaning(state)}
	
	def __str__(self):
		return "= " + str(self.left) + " " + str(self.right)

class Number( Expression ):
	def __init__(self, value):
		self.value = int(value)

	def meaning(self, state):
		return self.value
		
	def __str__(self):
		return str(self.value)

class String( Expression ):
	def __init__(self, value):
		self.value = value

	def meaning(self, state):
		return self.value
		
	def __str__(self):
		return self.value

class Ident( Expression ):
	def __init__(self, name):
		self.name = name

	def meaning(self, state):
		return state.get(self.name)

	def __str__(self):
		return self.name

class WhileStmt():
	def __init__(self, expr, block):
		self.expr = expr
		self.block = block

	def meaning(self, state):
		while(self.expr.meaning(state)):
			state = self.block.meaning(state)
		return state

	def __str__(self):
		return "while " + str(self.expr) + "\n" + str(self.block) + "\nendwhile"
		
class IfStmt():
	def __init__(self, expr, block1, block2=None):
		self.expr = expr
		self.block1 = block1
		self.block2 = block2

	def meaning(self, state):
		if (self.expr.meaning(state)):
			return self.block1.meaning(state)
		else:
			return self.block2.meaning(state)

	def __str__(self):
		if self.block2 != None:
			return "if " + str(self.expr) + "\n" + str(self.block1) + "\nelse\n" + str(self.block2) + "\nendif"
		else:
			return "if " + str(self.expr) + "\n" + str(self.block1) + "\nelse\nendif"


class StmtList():
	def __init__(self):
		self.lst = []
	
	def append(self, s):
		self.lst.append(s)

	def meaning(self, state):
		for e in self.lst:
			state = e.meaning(state)
		return state

	def __str__(self):
		return "\n".join([str(e) for e in self.lst])



def error( msg ):
	#print msg
	sys.exit(msg)

# The "parse" function. This builds a list of tokens from the input string,
# and then hands it to a recursive descent parser for the PAL grammar.

def match(matchtok):
	tok = tokens.peek()
	if (tok != matchtok): error("Expecting "+ matchtok)
	tokens.next()
	return tok
	
def factor():
	""" factor     = number | string | ident |  "(" expression ")" """

	tok = tokens.peek()
	if debug: print ("Factor: ", tok)
	if re.match(Lexer.number, tok):
		expr = Number(tok)
		tokens.next()
		return expr
	elif re.match(Lexer.string, tok):
		expr = String(tok)  # or match( tok )
		tokens.next()
		return expr
	elif re.match(Lexer.identifier, tok):
		expr = Ident(tok)  # or match( tok )
		tokens.next()
		return expr
	elif tok == "(":
		tokens.next()  # or match( tok )
		expr = expression()
		tokens.peek()
		tok = match(")")
		return expr
	error("Invalid operand")


def term():
	""" term    = factor { ('*' | '/') factor } """

	tok = tokens.peek()
	if debug: print ("Term: ", tok)
	left = factor()
	tok = tokens.peek()
	while tok == "*" or tok == "/":
		tokens.next()
		right = factor()
		left = BinaryExpr(tok, left, right)
		tok = tokens.peek()
	return left

def addExpr():
	""" addExpr    = term { ('+' | '-') term } """

	tok = tokens.peek()
	if debug: print ("addExpr: ", tok)
	left = term()
	tok = tokens.peek()
	while tok == "+" or tok == "-":
		tokens.next()
		right = term()
		left = BinaryExpr(tok, left, right)
		tok = tokens.peek()
	return left

def relationalExpr():
	""" relationalExpr = addExpr [ relation addExpr ] """

	left = addExpr()
	tok = tokens.peek()
	if re.match(Lexer.relational, tok):
		if debug: print("relational: relation: {}".format(tok))
		tokens.next()
		left = BinaryExpr(tok, left, addExpr())
	return left

def andExpr():
	""" andExpr    = relationalExpr { "and" relationalExpr } """

	left = relationalExpr()
	while (tokens.peek() == "and"):
		if debug: print("andExpr: and")
		tokens.next()
		left = BinaryExpr("and", left, relationalExpr())
	return left

def expression():
	""" expression = andExpr { "or" andExpr } """

	left = andExpr()
	while (tokens.peek() == "or"):
		if debug: print("expression: or")
		tokens.next()
		left = BinaryExpr("or", left, andExpr())
	return left


def assign():
	""" assign = ident "=" expression eoln """

	if re.match(Lexer.identifier, tokens.peek()):
		i = Ident(tokens.peek())
		if debug: print("assign {}".format(i))
		tokens.next()
		match("=")
		expr = expression()
		match(";")
		return Assign(i, expr)
	error("Illegal assign")


def block():
	""" block = ":" eoln indent stmtList undent """

	match(":")
	match(";") # eoln
	match("@") # indent

	lst = StmtList()
	while (tokens.peek() != "~"):
		lst.append(statement())

	match("~") # undent
	return lst


def whileStmt():
	""" whileStatement = "while"  expression  block """

	if debug: print("whileStmt")
	match("while")
	return WhileStmt(expression(), block())

def ifStmt():
	""" ifStatement = "if" expression block   [ "else" block ] """
	
	if debug: print("ifStmt")
	match("if")
	e = expression()
	b1 = block()
	if (tokens.peek() == "else"):
		if debug: print("ifStmt: else detected")
		tokens.next()
		b2 = block()
		return IfStmt(e, b1, b2)
	else:
		return IfStmt(e, b1)


def statement():
	""" statement = ifStatement |  whileStatement  |  assign """

	tok = tokens.peek()
	if (tok == "while"):
		return whileStmt()
	elif (tok == "if"):
		return ifStmt()
	else:
		return assign()

def stmtList():
	""" stmtList =  {  statement  } """
	lst = StmtList()
	while (tokens.peek() != None):
		lst.append(statement())
	return lst

def printState(state):
	i = ", ".join([f"<{k}, {v}>" for k,v in state.items()])
	print("{"+ i + "}")

def parse( text ) :
	global tokens
	tokens = Lexer( text )
	if debug: print("\n\n=================\n\n")
	script = stmtList()
	if debug: print("\n\n=================\n\n")
	print()
	printState( script.meaning({}))
	return


# Lexer, a private class that represents lists of tokens from a Gee
# statement. This class provides the following to its clients:
#
#   o A constructor that takes a string representing a statement
#       as its only parameter, and that initializes a sequence with
#       the tokens from that string.
#
#   o peek, a parameterless message that returns the next token
#       from a token sequence. This returns the token as a string.
#       If there are no more tokens in the sequence, this message
#       returns None.
#
#   o removeToken, a parameterless message that removes the next
#       token from a token sequence.
#
#   o __str__, a parameterless message that returns a string representation
#       of a token sequence, so that token sequences can print nicely

class Lexer :
	
	
	# The constructor with some regular expressions that define Gee's lexical rules.
	# The constructor uses these expressions to split the input expression into
	# a list of substrings that match Gee tokens, and saves that list to be
	# doled out in response to future "peek" messages. The position in the
	# list at which to dole next is also saved for "nextToken" to use.
	special = r"\(|\)|\[|\]|,|:|;|@|~|;|\$"
	relational = "<=?|>=?|==?|!="
	arithmetic = "\+|\-|\*|/"
	#char = r"'."
	string = r"'[^']*'" + "|" + r'"[^"]*"'
	number = r"\-?\d+(?:\.\d+)?"
	literal = string + "|" + number
	#idStart = r"a-zA-Z"
	#idChar = idStart + r"0-9"
	#identifier = "[" + idStart + "][" + idChar + "]*"
	identifier = "[a-zA-Z]\w*"
	lexRules = literal + "|" + special + "|" + relational + "|" + arithmetic + "|" + identifier
	
	def __init__( self, text ) :
		self.tokens = re.findall( Lexer.lexRules, text )
		self.position = 0
		self.indent = [ 0 ]
	
	
	# The peek method. This just returns the token at the current position in the
	# list, or None if the current position is past the end of the list.
	
	def peek( self ) :
		if self.position < len(self.tokens) :
			return self.tokens[ self.position ]
		else :
			return None
	
	
	# The removeToken method. All this has to do is increment the token sequence's
	# position counter.
	
	def next( self ) :
		self.position = self.position + 1
		return self.peek()
	
	
	# An "__str__" method, so that token sequences print in a useful form.
	
	def __str__( self ) :
		return "<Lexer at " + str(self.position) + " in " + str(self.tokens) + ">"



def chkIndent(line):
	ct = 0
	for ch in line:
		if ch != " ": return ct
		ct += 1
	return ct
		

def delComment(line):
	pos = line.find("#")
	if pos > -1:
		line = line[0:pos]
		line = line.rstrip()
	return line

def mklines(filename):
	inn = open(filename, "r")
	lines = [ ]
	pos = [0]
	ct = 0
	for line in inn:
		print(line[:-1])
		ct += 1
		line = line.rstrip()+";"
		line = delComment(line)
		if len(line) == 0 or line == ";": continue
		indent = chkIndent(line)
		line = line.lstrip()
		if indent > pos[-1]:
			pos.append(indent)
			line = '@' + line
		elif indent < pos[-1]:
			while indent < pos[-1]:
				del(pos[-1])
				line = '~' + line
		# print (ct, "\t", line)
		lines.append(line)
	# print len(pos)
	undent = ""
	for i in pos[1:]:
		undent += "~"
	lines.append(undent)
	# print undent
	return lines



def main():
	"""main program for testing"""
	global debug
	ct = 0
	for opt in sys.argv[1:]:
		if opt[0] != "-": break
		ct = ct + 1
		if opt == "-d": debug = True
	if len(sys.argv) < 2+ct:
		print ("Usage:  %s filename" % sys.argv[0])
		return
	parse("".join(mklines(sys.argv[1+ct])))
	return

if __name__ == '__main__':
	main()
