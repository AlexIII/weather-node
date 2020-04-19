/*
 * Weather Node Station
 * author: github.com/AlexIII
 * e-mail: endoftheworld@bk.ru
 * license: MIT
 */

import React from 'react';
import { StyleSheet, Text, View, ScrollView, RefreshControl } from 'react-native';
import { WNodeListItem } from './WNodeListItem';
import { SensorNameForm } from './SensorNameForm';
import { WeatherNode } from './SensorListScreen';
import { Touchable } from './Touchable';

export interface SensorListProps {
  nodes: Record<string, WeatherNode>;
  nodeNames: Record<string, string>;
  setNodeName: (mac: string, name: string) => void;
  scanError?: string;
  restartScan: () => void;
}

export const SensorList = ({nodes, nodeNames, setNodeName, scanError, restartScan}: SensorListProps) => {
  const [showModalSetName, setShowModalSetName] = React.useState<string>();

  const nodesSorted = React.useMemo(() => 
    Object.values(nodes)
      .sort((n1, n2) => (nodeNames[n1.mac] ?? n1.mac).localeCompare(nodeNames[n2.mac] ?? n2.mac))   //sord by name
      .sort((n1, n2) => Math.round(new Date(n2.updated).getTime() / 60000) - Math.round(new Date(n1.updated).getTime() / 60000)) //sort by 60min group
  , [nodes, nodeNames]);

  return (
    <>
      <ScrollView style={{flex: 1}} refreshControl={<RefreshControl refreshing={false} onRefresh={restartScan} />}>
        <View style={styles.sensorList}>
          {
            !scanError? null : <Text style={styles.errorText}>{`Bluetooth error: ${scanError}`}</Text>
          }
          {
            nodesSorted.map((node, idx) => (
              <Touchable key={`wnode-${idx}`} onPress={() => setShowModalSetName(node.mac)} >
                <View style={styles.sensorItem} > 
                  <WNodeListItem node={node} title={nodeNames[node.mac] ?? node.mac} /> 
                </View>
              </Touchable>
            ))
          }
          <Text>Scanning for devices...</Text>
        </View>
      </ScrollView>

      {!showModalSetName? null :
        <SensorNameForm 
          mac={showModalSetName} 
          name={nodeNames[showModalSetName] ?? ''} 
          onChange={name => {
            setNodeName(showModalSetName, name);
            setShowModalSetName(undefined);
          }}
        />
      }
    </>
  );
};

const styles = StyleSheet.create({
  sensorList: {
    flex: 1,
    alignItems: 'center',
    justifyContent: 'flex-start',
    marginTop: 10,
    marginBottom: 30,
    borderColor: '#000'
  },
  sensorItem: {
    marginBottom: 10,
    width: '100%',
    paddingLeft: 20,
    paddingRight: 20
  },
  errorText: {
    color: '#A00', 
    fontSize: 16, 
    fontWeight: 'bold', 
    paddingLeft: 20,
    paddingRight: 20,
    marginBottom: 10,
    textAlign: 'center'
  }
});
