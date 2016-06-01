import sys
import math

thrustNums = 4
timeInc = 100
shouldContinue = 1
first = 1

fts = []
itss = []
vals = []

#WILL BE CHANGED
def percToServ(perc):
    return int((10*perc)+1000);
    
def exitCheck(resp):
    return {
        "Y" : 1,
        "N" : 0,
        }[resp]

def pctCheck(pct):
    return {
        (pct > 100 or pct < 0) : 0,
        (pct <= 100 or pct >= 0) : 1,
        }[pct]

while shouldContinue:
    time = input("Enter the time interval through which the thruster valves must be actuated (in ms):")
    its = int(round(time/timeInc))
    if(not first):
        itss = fts[:]
        del vals[:]
    for i in range(0, thrustNums):
        if(first):
            itss.append(percToServ(input("Enter the desired initial thrust percent for valve " + str(i + 1) + "(0% - 100%):")))
            fts.append(percToServ(input("Enter the desired final thrust percent for valve " + str(i + 1) + "(0% - 100%):")))
        else:
            fts[i] = percToServ(input("Enter the desired final thrust percent for valve " + str(i + 1) + "(0% - 100%):"))
        vals.append([])
        inc = float(fts[i] - itss[i])/its
        for j in range(0, its):
            vals[i].append(itss[i] + int(math.floor(inc*(j+1))))

    if(first):
        open("out.txt","w")

    with open("out.txt", "a") as outfile:
        for j in range(0, its):
            for i in range(0, thrustNums):
                outfile.write("{}".format(vals[i][j]))
                if(i != 3):
                    outfile.write(",")

    iterate = 1
    outfile.close()
    
    while iterate:
        cont = raw_input("Add another iteration (Y/N)?")
        if(cont == "Y" or cont == "N" or cont == "y" or cont == "n"):
            shouldContinue = exitCheck(str(cont))
            iterate = 0
            first = 0
            
                
            
        


        
