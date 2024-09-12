import numpy as np
import matplotlib.pyplot as plt
import matplotlib.gridspec as gridspec
from matplotlib import rc
import pandas as pd
import sys
import csv

# y-axis in bold
rc('font')

fig, ax = plt.subplots(2,1, gridspec_kw={'height_ratios': [7,1]}, figsize=(12, 8))
fig.subplots_adjust(hspace=0.05)

#plt.subplots(gridspec_kw={'height_ratios':[1]})

for iteration in range(0,1):

    trace_list_id = iteration
    trace_list_name = ""
    trace_list_prefetcher_order = [] 

    
    trace_name = []
    l1i = []
    l1d = []
    l2c = []
    llc = []
    interconnect = []
    dram = []
    tlbs = []
    prefetcher = []

    barWidth = 0.004

    r = [0, 0.004,0.008,0.012,0.016,0.02]

    static_energy_file = open("static_energy.csv", "r")
    count = 0
    content=csv.reader(static_energy_file,delimiter=",") 
    end = 0
    for row in content:
        if(len(row) >= 1): 
            if(count%9 == 0):
                #Trace_name
                if(count > 0):
                    #print("Plotting: "+trace_name[len(trace_name)-1])
                    #plot 
                    #red, yellow, pink, green, orange, blue 
                    p1 = ax[1].bar(r, l1i, color='#F7DC6F', width=barWidth, align='center', edgecolor = "k", linewidth=0.8)
                    p2 = ax[1].bar(r, l1d, bottom=np.array(l1i), color='#CD6155', width=barWidth, align='center', edgecolor = "k", linewidth=0.8)
                    p13 = ax[1].bar(r, tlbs, bottom=np.array(l1i)+np.array(l1d), hatch = '+++', color='white', width=barWidth, align='center', edgecolor = "k", linewidth=0.8)
                    p3 = ax[1].bar(r, l2c, bottom=np.array(l1i)+np.array(l1d)+np.array(tlbs), color='#95A5A6', width=barWidth, align='center', edgecolor = "k", hatch = '///', linewidth=0.8)
                    p4 = ax[1].bar(r, interconnect, bottom=np.array(l1i)+np.array(l1d)+np.array(tlbs)+np.array(l2c), color='#2E4053', width=barWidth, align='center', edgecolor = "k", linewidth=0.8)
                    p5 = ax[1].bar(r, llc, bottom=np.array(l1i)+np.array(l1d)+np.array(tlbs)+np.array(l2c)+np.array(interconnect), color='#8E44AD', width=barWidth, align='center', edgecolor = "k", hatch = '\\\\\\', linewidth=0.8)
                    p6 = ax[1].bar(r, dram, bottom=np.array(l1i)+np.array(l1d)+np.array(tlbs)+np.array(l2c)+np.array(interconnect)+np.array(llc), color='#EDBB99', width=barWidth, align='center', edgecolor = "k", linewidth=0.8)
                    p14 = ax[1].bar(r, prefetcher, bottom=np.array(l1i)+np.array(l1d)+np.array(tlbs)+np.array(l2c)+np.array(interconnect)+np.array(llc)+np.array(dram), color='white', width=barWidth, align='center', edgecolor = "k", linewidth=0.8)


                    p1 = ax[0].bar(r, l1i, color='#F7DC6F', width=barWidth, align='center', edgecolor = "k", linewidth=0.8)

                    p2 = ax[0].bar(r, l1d, bottom=np.array(l1i), color='#CD6155', width=barWidth, align='center', edgecolor = "k", linewidth=0.8)
                    p13 = ax[0].bar(r, tlbs, bottom=np.array(l1i)+np.array(l1d), hatch = '+++', color='white', width=barWidth, align='center', edgecolor = "k", linewidth=0.8)

                    p3 = ax[0].bar(r, l2c, bottom=np.array(l1i)+np.array(l1d)+np.array(tlbs), color='#95A5A6', width=barWidth, align='center', edgecolor = "k", hatch = '///', linewidth=0.8)
        

                    p4 = ax[0].bar(r, interconnect, bottom=np.array(l1i)+np.array(l1d)+np.array(tlbs)+np.array(l2c), color='#2E4053', width=barWidth, align='center', edgecolor = "k", linewidth=0.8)
                    
                    p5 = ax[0].bar(r, llc, bottom=np.array(l1i)+np.array(l1d)+np.array(tlbs)+np.array(l2c)+np.array(interconnect), color='#8E44AD', width=barWidth, align='center', edgecolor = "k", hatch = '\\\\\\', linewidth=0.8)
                    
                    p6 = ax[0].bar(r, dram, bottom=np.array(l1i)+np.array(l1d)+np.array(tlbs)+np.array(l2c)+np.array(interconnect)+np.array(llc), color='#EDBB99', width=barWidth, align='center', edgecolor = "k", linewidth=0.8)
                    
                    p14 = ax[0].bar(r, prefetcher, bottom=np.array(l1i)+np.array(l1d)+np.array(tlbs)+np.array(l2c)+np.array(interconnect)+np.array(llc)+np.array(dram), color='white', width=barWidth, align='center', edgecolor = "k", linewidth=0.8)

                    ax[0].axhline(y=1, color='red', linestyle='--', linewidth = 0.5)
                    
                    ax[1].set_ylim(0,0.1)
                    ax[1].set_yticklabels(['0','0.1'], fontsize=9.5)
                    ax[0].set_ylim(0.92,1.1)
                    ax[0].set_yticks(np.arange(0.95, 1.1, 0.05))
                    ax[0].set_yticklabels(['0.95','1','1.05','1.1'],fontsize=9.5)

                    ax[0].spines['bottom'].set_visible(False)
                    ax[1].spines['top'].set_visible(False)
                    ax[0].xaxis.tick_top()
                    ax[0].tick_params(labeltop='off')
                    ax[1].xaxis.tick_bottom()

                    #clear lists trace name and energy, update r 
                    l1i.clear()
                    l1d.clear()
                    l2c.clear()
                    llc.clear()
                    interconnect.clear()
                    dram.clear()
                    tlbs.clear()
                    prefetcher.clear()
                    for index in range(0,len(r)):
                        r[index] = r[index] + barWidth*6 + 0.005
                        
                

                if(row[0].find("end") != -1):
                    #print("Count: "+str(count))
                    break

                trace_name.append(row[0].strip());
                #print("Inserting: "+str(row[0].strip()))
            else:
                #print("Getting data "+trace_name[len(trace_name)-1])
                if(row[0].find("L1I") != -1):
                    for index in range(1,7):
                        l1i.append(float(row[index].strip()));
                elif(row[0].find("L1D") != -1):
                    for index in range(1,7):
                        l1d.append(float(row[index].strip()));
                elif(row[0].find("L2C") != -1):
                    for index in range(1,7):
                        l2c.append(float(row[index].strip()));
                elif(row[0].find("LLC") != -1):
                    for index in range(1,7):
                        llc.append(float(row[index].strip()));
                elif(row[0].find("Interconnect") != -1):
                    for index in range(1,7):
                        interconnect.append(float(row[index].strip()));
                elif(row[0].find("DRAM") != -1):
                    for index in range(1,7):
                        dram.append(float(row[index].strip()));
                elif(row[0].find("TLBs") != -1):
                    for index in range(1,7):
                        tlbs.append(float(row[index].strip()));
                elif(row[0].find("Prefetcher") != -1):
                    for index in range(1,7):
                        prefetcher.append(float(row[index].strip()));

            count = count + 1;
        if(row[0] == "end:"):
            end = 1 



ax[0].grid(color='lightgrey', which='major', axis='y', linestyle='solid', zorder = 0)

ax[1].grid(color='lightgrey', which='major', axis='y', linestyle='solid', zorder = 0)

ax[0].set_axisbelow(True)
ax[1].set_axisbelow(True)


d = .5  # proportion of vertical to horizontal extent of the slanted line
kwargs = dict(marker=[(-1, -d), (1, d)], markersize=6, linestyle="none", color='k', mec='k', mew=1, clip_on=False)
ax[0].plot([0, 1], [0, 0], transform=ax[0].transAxes, **kwargs)
ax[1].plot([0, 1], [1, 1], transform=ax[1].transAxes, **kwargs)

ax[0].legend((p1[0], p2[0], p13[0], p3[0], p5[0], p6[0], p14[0]), ("L1I","L1D", "TLBs" ,"L2", "L3", "DRAM", "Prefetcher"), fontsize=9.5, handlelength=0.8, handletextpad = 0.5, labelspacing=0.2, columnspacing = 0.5, ncol=8, framealpha=0, fancybox=True, loc = (0.05,1.26))

ax[0].text(-0.004,1.2,"A: No Prefetcher, B: SPP, C: PPF, D: IPCP L1D, E: IPCP, F: Bingo",fontsize=9.5)

ax[1].text(-0.026, 0.1, "Static Energy", rotation='vertical', fontsize=9.5)
ax[0].set_xticklabels([])
ax[0].tick_params(axis='x',length=0)
ax[0].set_xlim(-0.0075,0.144)
ax[1].set_xlim(-0.0075,0.144)
ax[1].set_xticks(np.arange(0.01,0.15,0.029))
ax[1].set_xticklabels(trace_name, rotation = 0, fontsize = 9.2)


ax[1].text(-0.027, -0.30, "#Critical IPs:",fontsize=9.5, color='red')
ax[1].text(-0.027, -0.41, "#All IPs:",fontsize=9.5, color='blue')

ax[1].text(0.006, -0.30, "156",fontsize=9.5, color='red')
ax[1].text(0.0027, -0.41, "10875",fontsize=9.5, color='blue')

ax[1].text(0.037, -0.30, "7",fontsize=9.5, color='red')
ax[1].text(0.0328, -0.41, "32247",fontsize=9.5, color='blue')

ax[1].text(0.066, -0.30, "3",fontsize=9.5, color='red')
ax[1].text(0.061, -0.41, "13263",fontsize=9.5, color='blue')

ax[1].text(0.095, -0.30, "0",fontsize=9.5, color='red')
ax[1].text(0.091, -0.41, "8589",fontsize=9.5, color='blue')

ax[1].text(0.124, -0.30, "2",fontsize=9.5, color='red')
ax[1].text(0.124, -0.41, "9",fontsize=9.5, color='blue')


plt.subplots_adjust(wspace=0.06, hspace=0.1, right = 0.48, top = 0.30, bottom = 0.18)

#plt.savefig('SavingPlots/pdfs/Static_Energy_Workloads.pdf', bbox_inches = 'tight', pad_inches = 0.1)
#plt.savefig('SavingPlots/pngs/Static_Energy_Workloads.png', bbox_inches = 'tight', pad_inches = 0.1)
plt.show()
