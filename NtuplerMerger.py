from ROOT import TChain
import glob
import xml.etree.cElementTree as ET
import os
from datetime import datetime


# Define time variable
now = datetime.now()
dd = str(now.day)
mm = str(now.month)
yyyy = str(now.year)
date = dd+"_"+mm+"_"+yyyy

#Define path where ntuples are stored
pathNonMerged = "MACRO_Output_MuEl/"  #needs to be changed for different lepton channel
pathMerged = "MergedTrees/"+date+"/"

if not os.path.exists(pathMerged):
    os.makedirs(pathMerged)


# get filenames from the xml!!!
tree = ET.ElementTree(file='config/FullMcBkgdSamplesV8.xml')

root = tree.getroot()
datasets = root.find('datasets')
print "found  "  + str(len(datasets)) + " datasets"
datasetNames = []

# loop over the datasets to be added and fill the "topTrees" vector
for d in datasets:
    if d.attrib['add'] == '1':
        print "found dataset to be added..." + str(d.attrib['name'])
        datasetNames.append(str(d.attrib['name']))

for n in datasetNames:
    filenames = glob.glob(pathNonMerged + "/*" + n + "*.root")
    hadd = "hadd " + pathMerged + "DisplacedTop_Run2_TopTree_Study_" + n + "_MuEl" + ".root"
    for f in filenames:
       hadd = hadd + " " + f
    print "Merging ntuples for " + n
    os.system(hadd)
