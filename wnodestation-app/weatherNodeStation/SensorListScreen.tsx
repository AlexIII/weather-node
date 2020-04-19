/*
 * Weather Node Station
 * author: github.com/AlexIII
 * e-mail: endoftheworld@bk.ru
 * license: MIT
 */

import React from 'react';
import { StyleSheet, Text, View, Image, Alert } from 'react-native';
import { BleManager, Device as BleDevice, BleError } from 'react-native-ble-plx';
import { Buffer } from 'buffer';
import AsyncStorage from '@react-native-community/async-storage';
import { SensorList } from './SensorList';
import { MenuProvider } from 'react-native-popup-menu';
import {
  Menu,
  MenuOptions,
  MenuOption,
  MenuTrigger,
} from 'react-native-popup-menu';
const appVersion: string = require('../package.json').version;

export interface WeatherNode {
  mac: string;
  name: string;
  temperature: number;
  humidity: number;
  batteryLevel?: 'high' | 'medium-high' | 'medium-low' | 'low';
  error?: string;
  updated: string;
}

export const SensorListScreen = () => {
  const [nodes, setNondes] = React.useState<Record<string, WeatherNode>>({});
  const [nodeNames, setNodeNames] = React.useState<Record<string, string>>({});
  const [scanError, setScanError] = React.useState<string>();
  const [scanFlag, setScanFlag] = React.useState({});

  const restartScan = React.useCallback(() => setScanFlag({}), []);
  const addNode = (node: WeatherNode, name?: string) => {
    setNondes(nodes => ({...nodes, [node.mac]: node}));
    if(name) setNodeNames(names => ({...names, [node.mac]: name}));
  };
  const setNodeName = (mac: string, name: string) => {
    if(name.length == 0) {
      setNodeNames(names => {
        const cpy = {...names};
        delete cpy[mac];
        return cpy;
      });
    } else setNodeNames(names => ({...names, [mac]: name}))
  };

  //save and resore node names and vaules
  React.useEffect(() => {
    AsyncStorage.getItem('node-names').then(v => v && setNodeNames(JSON.parse(v)));
    AsyncStorage.getItem('nodes').then(v => v && setNondes(JSON.parse(v)));
    return () => {
      setNodeNames(names => (AsyncStorage.setItem('node-names', JSON.stringify(names)), names));
      setNondes(nodes => (AsyncStorage.setItem('nodes', JSON.stringify(nodes)), nodes));
    }
  }, []);
/*
  //generate fake sensors
  React.useEffect(() => {
    const fakeNodes = 3;
    const fakeMACs = Array.from(Array(fakeNodes)).map(() => genFakeMac());
    const id = setInterval(() => fakeMACs.map((mac, i) => addNode(genFakeNode(mac), "Fake Sensor " + i)), 5000);
    return () => clearInterval(id);
  }, []);
*/
  //scan effect
  React.useEffect(() => {
    setScanError(undefined);
    
    const stopScan = scanBleWeatherNodes((error, device) => {
      if(error) {
        console.log(error);
        setScanError(error.message);
        return;
      }

      if(device?.name?.startsWith("wNode") && device.manufacturerData) {
        const mbuff = new Buffer(device.manufacturerData, 'base64');
        if(mbuff.length >= 8 && mbuff[0] == 0xA9 &&  mbuff[1] == 0x53) {
          const toInt16 = (byteH: number, byteL: number) => (((byteH & 0x7F) << 8) + byteL) * ((byteH & 0x80)? -1 : 1);
          const humidity = toInt16(mbuff[2], mbuff[3])/10;
          const temperature = toInt16(mbuff[4], mbuff[5])/10;
          const flags = mbuff[6];
          const batteryLevel = ['high', 'medium-high', 'medium-low', 'low'][flags&0x3] as any;
          const error = flags&0x4? 'sensor failure' : undefined;

          console.log(device.id, device.name, temperature, humidity, flags, batteryLevel);
          
          addNode({
            mac: device.id,
            name: device.name,
            temperature,
            humidity,
            batteryLevel,
            error,
            updated: new Date().toString()
          });
        }
      }
    });

    return stopScan;
  }, [scanFlag]);

  const clearSensorsAlert = React.useCallback(() => {
    Alert.alert(
      'Confirmation',
      'Delete all sensors?',
      [
        { text: 'Cancel', style: 'cancel' },
        { text: 'OK', onPress: () => { setNondes({}); setNodeNames({}); } }
      ],
      {cancelable: true}
    );
  }, []);

  return (<MenuProvider>
    <View style={styles.container}>

      {/* Header */}
      <View style={styles.header}>
        <Image style={{width: 30, height: 30, resizeMode: 'contain', marginRight: 15}} source={require('./icons/wns-icon.png')} />
        <Text style={{fontSize: 24, color: '#EEE', textAlign: 'center', marginRight: 15}}>Weather Node Station</Text>

        <Menu>
          <MenuTrigger>
            <Image style={{width: 30, height: 30, resizeMode: 'contain', marginRight: 5}} source={require('./icons/ellipsis-v.png')} />
          </MenuTrigger>
          <MenuOptions>
            <MenuOption onSelect={clearSensorsAlert} >
              <Text style={styles.menuOptionText}>Clear sensors</Text>
            </MenuOption>
            <MenuOption onSelect={() => Alert.alert(`About`, `Weather Node Station ${appVersion}`)} >
              <Text style={styles.menuOptionText}>About</Text>
            </MenuOption>
          </MenuOptions>
        </Menu>

      </View>

      {/* Sensor List */}
      <SensorList nodes={nodes} nodeNames={nodeNames} setNodeName={setNodeName} scanError={scanError} restartScan={restartScan}/>

    </View>
  </MenuProvider>);
};

const styles = StyleSheet.create({
  container: {
    backgroundColor: '#03E3',
    flex: 1,
    alignItems: 'center',
    justifyContent: 'flex-start',
  },
  header: {
    backgroundColor: '#03EA', 
    padding: 10, 
    width: '100%', 
    flex: 0.04, 
    flexDirection: 'row', 
    alignItems: 'center', 
    justifyContent: 'space-between'
  },
  menuOptionText: {
    fontSize: 20, 
    color: '#446'
  }
});

const scanBleWeatherNodes = (onScan: (error: BleError | null, device: BleDevice | null) => void) => {
  const bleManager = new BleManager();
  bleManager.startDeviceScan(
    null, //UUIDs: ?Array<UUID>,
    null, //options: ?ScanOptions,
    onScan
  );
  return () => bleManager.stopDeviceScan();
};

const genFakeMac = () => "00:11:22:CC:" + Math.random().toString().substr(2,2) + ":" + Math.random().toString().substr(2,2);
const genFakeNode = (mac: string): WeatherNode => {
  const temperature = Math.round(Math.random()*10000)/100;
  const humidity = Math.round(Math.random()*10000)/100;
  const batteryLevel = ['high', 'medium-high', 'medium-low', 'low'][Math.round(Math.random()*10)%4] as any;
  return {
    mac: mac,
    name: "wNode0",
    temperature: temperature,
    humidity: humidity,
    error: Math.random() < 0.1? 'sensor failure' : undefined,
    batteryLevel,
    updated: new Date().toString()
  };
};
