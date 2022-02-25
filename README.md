# Sensor History Reading

## Overview

A Sensor Reader is a daemon which handles Sensor History Reading operations.
When the Sensor Reader daemon comes up, it should create objects such as
`xyz.openbmc_project.SensorReader` 

# DbusObjects

## 1) Sensor History Objects

 `/xyz/openbmc_project/SensorReader/History`

## 2) Interface Objects

 `xyz.openbmc_project.SensorReader.History.Read`

# UseCases

### 1) Sensor Reader Tree
busctl tree xyz.openbmc_project.SensorReader
```
`-/xyz
  `-/xyz/openbmc_project
    `-/xyz/openbmc_project/SensorReader
      `-/xyz/openbmc_project/SensorReader/History
```

### 2) Sensor Reader Introspect
busctl introspect xyz.openbmc_project.SensorReader
                  /xyz/openbmc_project/SensorReader/History
```
NAME                                      TYPE    SIGNATURE  RESULT/    FLAGS
                                                             VALUE FLAGS
org.freedesktop.DBus.Introspectable       interface    -       -         -
.Introspect                               method       -       s         -
org.freedesktop.DBus.Peer                 interface    -       -         -
.GetMachineId                             method       -       s         -
.Ping                                     method       -       -         -
org.freedesktop.DBus.Properties           interface    -       -         -
.Get                                      method       ss      v         -
.GetAll                                   method       s      a{sv}      -
.Set                                      method       ssv     -         -
.Interval                                 property     t      60 emits-change
                                                                    writable
.TimeFrame                                property     t      10 emits-change
                                                                    writable
```

Note:
1. Interval is in seconds 
2. TimeFrame is in minutes

### 3) Configuring/Reading of Time Frame 
busctl set-property xyz.openbmc_project.SensorReader
           /xyz/openbmc_project/SensorReader/History
        xyz.openbmc_project.SensorReader.History.Read TimeFrame t < TimeFrame Value > 

busctl get-property xyz.openbmc_project.SensorReader
           /xyz/openbmc_project/SensorReader/History
        xyz.openbmc_project.SensorReader.History.Read TimeFrame
 ```
 t 10
 ```
### 4) Configuring/Reading of Interval 
busctl set-property xyz.openbmc_project.SensorReader
           /xyz/openbmc_project/SensorReader/History
        xyz.openbmc_project.SensorReader.History.Read Interval t < Interval Value > 
 
 busctl get-property xyz.openbmc_project.SensorReader
           /xyz/openbmc_project/SensorReader/History
        xyz.openbmc_project.SensorReader.History.Read Interval
 ```
 t 60
 ```
### 5) Sensor History Reading By Sensor Name
busctl call xyz.openbmc_project.SensorReader
           /xyz/openbmc_project/SensorReader/History
        xyz.openbmc_project.SensorReader.History.Read Read s "< Sensor Name>" 

```
a{td} 10 69861 0.903 69921 0.903 69981 0.903 70041 0.903 70101 0.903
         70161 0.903 70221 0.903 70281 0.903 70341 0.903 70401 0.903
```
Note: 
1.	The user needs to get “Sensor Name” from the object mapper 
2.	Output format for read

```
    Array  ArraySize  Timestamp  SensorValue
    a{td}     10       69861       0.903
```

## To Build
```
To build this package, do the following steps:

    1. meson build
    2. ninja -C build
```


