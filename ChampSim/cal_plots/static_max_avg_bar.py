# importing package
import matplotlib.pyplot as plt
import numpy as np
  
# create data
x = np.arange(5)
#y1 = [34, 56, 12, 89, 67]
#y2 = [12, 56, 78, 45, 90]
width = 0.32

#SPP, PPF, IPCP L1, IPCP, Bingo
avg_static = [0.9035560285, 0.8752115996, 0.8811979681, 0.8760561704, 0.8622118812]
max_static = [1.257538101, 1.204709538, 1.196122802, 1.227371777, 1.195352342]

fig, ax = plt.subplots(figsize=(12,8))

# plot data in grouped manner of bar type
b1 = ax.bar(x-0.2, avg_static, width, color = "#D1C4E9", edgecolor = "k", linewidth=0.8, hatch = '///')
b2 = ax.bar(x+0.2, max_static, width, color = "#FFAB91", edgecolor = "k", linewidth=0.8)

ax.set_ylim(0.8, 1.3)

ax.xaxis.set_ticks(x)
ax.set_xticklabels(("SPP", "PPF", "IPCP\nL1D", "IPCP", "Bingo"), fontsize = 9.5, rotation=90, linespacing=0.9);
ax.axhline(y=1, color='red', linestyle='--', linewidth = 0.5)
ax.grid(color='lightgrey', which='major', axis='y', linestyle='solid', zorder = 0)
ax.set_axisbelow(True)

ax.set_ylabel("Normalized\nStatic Energy", fontsize = 9.5)

ax.set_yticklabels(['0.8','1','1.2'])

ax.legend((b1[0], b2[0]), ("Average","Maximum"), fontsize=9.5, handlelength=0.8, handletextpad = 0.5, labelspacing=0.2, columnspacing = 0.5, ncol=2, framealpha=0, fancybox=True, loc = (-0.14,1))


plt.subplots_adjust(wspace=0.33, hspace=0, right = 0.24, top = 0.25)

#plt.savefig('SavingPlots/pdfs/Static_Avg_Max.pdf', bbox_inches = 'tight', pad_inches = 0.1)
#plt.savefig('SavingPlots/pngs/Static_Avg_Max.png', bbox_inches = 'tight', pad_inches = 0.1)
plt.show()
