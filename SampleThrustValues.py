import sys
import math

thrust_nums = 4

time = input("Enter the time interval through which the thruster valves must be actuated (in ms):")

its = int(round(time/10))

fts = []
vals = []

for i in range(0, thrust_nums):
    fts.append(input("Enter the desired final servo value for valve " + str(i + 1) + "(0-2000):"))
    vals.append([])
    inc = float(fts[i])/its
    for j in range(0, its):
        vals[i].append(int(math.ceil(inc*(j+1))))

open("out.txt","w")

with open("out.txt", "a") as outfile:
    for j in range(0, its):
        for i in range(0, thrust_nums):
            outfile.write("{}".format(vals[i][j]))
            if(i != 3):
                outfile.write(",")
        outfile.write("\n")


#insert definition for thrust-to-motor position equation
        
