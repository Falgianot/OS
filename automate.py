import os
import sys
import timeit
import subprocess



#Grabs the inputs from the users, specifically checking to make sure they enter a valid integer, which it will later run n times
def entry():
	terminalEntry = raw_input("Welcome to the time automation script! Please enter your terminal command: ")
	while 1:
		iterationCount = input("How many times would you like to run it?: ")
		try:
	    		iterationCount += 1
			iterationCount -= 1
		except TypeError:
			print "Invalid iteration number, please try again..."
			continue
		if iterationCount < 0:
			print "Invalid iteration number, please try again..."
			continue
		else:
			totalIteration(terminalEntry,iterationCount)
			break



#Runs through the test case n times, collecting the real time as it iterates, then computes the average time of completion of the C program
def totalIteration(terminalEntry,iterationCount):
	arr = [None]*iterationCount
	i = 0
	while i < iterationCount:
		process = subprocess.Popen([terminalEntry], stderr=subprocess.PIPE, shell=True)
		endTime = getThatTime(process.stderr.read())
		#timeStr = ''.join(pulledTime)
		#endTime = float(timeStr)
		arr[i] = endTime
		print "~~",endTime," sec~~"
		i += 1
	i = 0
	sum1 = 0
	while i < iterationCount:
		sum1 += arr[i]
		i += 1
	average = sum1/iterationCount
	time1 = convertToMinandSec(average)
	time2 = convertToMinandSec(sum1)
	print "Average Time: ",time1
	print "Accumulated Time Across All Executions: ",time2



#Converts and prints to a '0 min | 0 sec' format, taking in the full amount of seconds as a parameter
def convertToMinandSec(endNum):
	minutes = int(endNum // 60)
	seconds = endNum % 60
	return "" + str(minutes) + " min | " + str(seconds) + " sec"



#Simple check to see if something is a number. If it is, it returns it casted as a float; if not, it returns -1
def isNum(string):
	try:
   		number = float(string)
		return number
	except ValueError:
   		return -1



#Splits up the stdout (though actually stderr) stream to pull the real time for average speed computation
def getThatTime(stdout):
	stdoutList = stdout.split('\n')
	for token in stdoutList:
		tokenList = token.split()
		for tok in tokenList:
			length = len(tok)
			if length > 0:
				if tok[0] != 'r':
					i = 0
					minuteNum = 0
					secondNum = 0
					while i < length:
						if tok[i] == 'm':
							j = i + 1
							numArr = []
							while tok[j] != 's':
								numArr.append(tok[j])
								j += 1
							numArrJoined = ''.join(numArr)
							secondNum = isNum(numArrJoined)
							return minuteNum + secondNum
						elif isNum(tok[i]) > -1:
							minuteNum = isNum(tok[i]) * 60
						i += 1
	

def main():
	entry()


main()
