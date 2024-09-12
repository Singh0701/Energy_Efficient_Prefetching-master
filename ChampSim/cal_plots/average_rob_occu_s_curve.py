import numpy as np
import matplotlib.pyplot as plt
from matplotlib.patches import Ellipse, Circle
import csv 

critical_points = []
non_critical_points = []
xtick_marker = []
count = 1

with open('average_ROB_occupancy_90_percent_stalling_ips.csv','r') as csvfile:
    content=csv.reader(csvfile,delimiter=",")
    for row in content:
        if(row[0] == "end:"):
            continue
        if(len(row) >= 1):
            non_critical_points.append(float(row[1]))
            critical_points.append(float(row[0]))
            if(float(row[0]) < float(row[1])):
                xtick_marker.append(count)
            count = count + 1
            
        else:
            break





X = []
for i in range(len(critical_points)):
    X.append(i)

fig, ax = plt.subplots(ncols=1, figsize=(12, 8))



critical = plt.plot(X, critical_points, color = '#2E86C1')
non_critical = plt.plot(X, non_critical_points, color = '#404040', linestyle = 'dashed')




plt.ylim(0, 352) 

plt.ylabel("Average ROB Occupancy", fontsize = 9.5)

plt.grid(color='lightgrey', which='major', axis='y', linestyle='solid', zorder = 0)

ax.legend((critical[0], non_critical[0]), ("Average ROB occupancy for IPs causing 90% of ROB stalls", "Average ROB occupancy for IPs causing remaining ROB stalls"), handlelength=1.5, handletextpad = 0.5, labelspacing=0.2, ncol=1, framealpha=0, fancybox=True, loc = (-0.05, 1), fontsize=9.5)
       

ax.margins(x=0.01)




ax.arrow(0,-18,40,0, clip_on = False, width = 2, head_width = 18, head_length = 3.5, color = 'black')
ax.arrow(39,-18,-35,0, clip_on = False, width = 2, head_width = 18, head_length = 3.5, color = 'black', head_starts_at_zero = True)
ax.text(10, -58, "SPEC CPU 2017", fontsize = 9.5)

ax.arrow(46,-18,32,0, clip_on = False, width = 2, head_width = 18, head_length = 3.5, color = 'black')
ax.arrow(79,-18,-29,0, clip_on = False, width = 2, head_width = 18, head_length = 3.5, color = 'black', head_starts_at_zero = True)
ax.text(53, -58, "Client/Server", fontsize = 9.5)

ax.arrow(83,-18,4.5,0, clip_on = False, width = 2, head_width = 18, head_length = 2, color = 'black')
ax.arrow(89,-18,-3,0, clip_on = False, width = 2, head_width = 18, head_length = 2, color = 'black', head_starts_at_zero = True)


ax.text(77, -60, "Cloudsuite", fontsize = 9.5)

#ax.set_yticklabels(['0','100','200','300'], fontsize=9.5)


plt.subplots_adjust(top = 0.27, right = 0.47)

plt.tick_params(axis='x',length=0)
plt.xticks([])
#plt.savefig('SavingPlots/pdfs/average_ROB_occu.pdf', bbox_inches = 'tight', pad_inches = 0.1)
#plt.savefig('SavingPlots/pngs/average_ROB_occu.png', bbox_inches = 'tight', pad_inches = 0.1)
plt.show()
