# importing package
import matplotlib.pyplot as plt
import numpy as np
  
# create data
x = np.arange(5)
width = 0.32

#SPP, PPF, IPCP L1, IPCP, Bingo
avg_dynamic = [1.054027911, 1.063901782, 1.061386913, 1.073792654, 1.03904816]
max_dynamic = [1.374967293, 1.750338696, 2.240907191, 2.241874131, 1.522920605]

fig, ax = plt.subplots(figsize=(12,8))

# plot data in grouped manner of bar type
b1 = ax.bar(x-0.2, avg_dynamic, width, color = "#90CAF9", edgecolor = "k", linewidth=0.8, hatch = '///')
b2 = ax.bar(x+0.2, max_dynamic, width, color = "#EF9A9A", edgecolor = "k", linewidth=0.8)

ax.set_ylim(1, 1.4)
#ax.set_yscale('log')
ax.xaxis.set_ticks(x)
ax.set_xticklabels(("SPP", "PPF", "IPCP\nL1D", "IPCP", "Bingo"), fontsize = 9.5, rotation=90, linespacing=0.9);
ax.axhline(y=1, color='red', linestyle='--', linewidth = 0.5)
ax.grid(color='lightgrey', which='major', axis='y', linestyle='solid', zorder = 0)
ax.set_axisbelow(True)

ax.set_ylabel("Normalized\nDynamic Energy", fontsize = 9.5)

ax.set_yticklabels(['1','1.1','1.2','1.3','1.4'])
ax.text(1.8,1.41,'2.2',fontsize=9.5)
ax.text(2.9,1.41,'2.2',fontsize=9.5)
ax.text(0.5,1.41,'1.75',fontsize=9.5)
ax.text(3.9,1.41,'1.52',fontsize=9.5)
ax.legend((b1[0], b2[0]), ("Average","Maximum"), fontsize=9.5, handlelength=0.8, handletextpad = 0.5, labelspacing=0.2, columnspacing = 0.5, ncol=2, framealpha=0, fancybox=True, loc = (-0.14,1.1))


plt.subplots_adjust(wspace=0.33, hspace=0, right = 0.24, top = 0.25)

#plt.savefig('SavingPlots/pdfs/Dynamic_Avg_Max.pdf', bbox_inches = 'tight', pad_inches = 0.1)
#plt.savefig('SavingPlots/pngs/Dynamic_Avg_Max.png', bbox_inches = 'tight', pad_inches = 0.1)
plt.show()
