import sys

idle = True
header = True

# holds one line of data formated as 'Throughput', 'Node-0 consumption', 
# 'Node-1 Consumption', 'Node-2 Consumption', 'Total Consumption'

data = [] 
total_energy = 0.0 # total energy consumped by node-0, node-1 and node-2

with open('flow_energy.dat') as fe:
    for line in fe:
       if line.startswith("Time"): # ignores unnecessary lines in the input file
          list = line.split()
          if idle: # extract the power consumption data at time 0 when the nodes are idle
              if list[1]=="0" and list[3]!="2":
                 data.append(list[5])
                 total_energy += float(list[5])
              elif list[1]=="0" and list[3]=="2":
                 idle=False
                 data.insert(0, "0.0")
                 data.append(list[5])
                 total_energy += float(list[5])
                 data.append(str(total_energy))
          else: # extract the rest of power consumption and throughput data
              if list[1]!="0" and list[2]!="Throughput":
                 data.append(list[5])
                 total_energy += float(list[5])
              elif list[2]=="Throughput":
                 data.insert(0,list[3])
                 data.append(str(total_energy))
          with open('flow_energy_output.dat',"a") as of:
              if len(data) == 5:
                  if header: # writes the header and the idle consumption
                      of.write("%s\n" % "Throughput,Cons_Node0,Cons_Node1,Cons_Node2,Cons_Total")
                      of.write("%s\n" % ",".join(data))
                      header = False
                      del data[:]
                  else: # writes the rest of the consumption
                      of.write("%s\n" % ",".join(data))
                      del data[:]
       elif line.startswith("Lost"): # if there are packet losses, stop processing the file
           sys.exit()
         
