/*
 * Weather Node Station
 * author: github.com/AlexIII
 * e-mail: endoftheworld@bk.ru
 * license: MIT
 */

import React from 'react';
import { StyleSheet, Text, View, Image } from 'react-native';
import { WeatherNode } from './SensorListScreen';
import { format } from 'date-fns';

export interface WeatherNodeListItemProps {
  node: WeatherNode;
  title: string;
};

export const WNodeListItem = ({node, title}: WeatherNodeListItemProps) => {
  return (
    <View style={styles.item}>
      <View style={{width: '100%', flex: 1, flexDirection: 'row', justifyContent: 'space-between'}}>
        <Text style={styles.sensorHeaderText}>{title}</Text>
        <View style={{flexDirection: 'row'}}>  
          {
            !node.error
              ? <Text style={styles.sensorHeaderText}><UpdDate date={new Date(node.updated)} /></Text>
              : <Text style={styles.sensorHeaderTextError}>{ `Error: ${node.error}` }</Text>
          }
          <BatteryIcon batteryLevel={node.batteryLevel} />
        </View>
      </View>
      <View style={{width: '100%', flex: 1, flexDirection: 'row'}}>
        <View style={{width: '100%', flex: 1, flexDirection: 'row'}}>
          <Image style={{width: 30, height: 30, resizeMode: 'contain'}} source={require('./icons/temp-icon.png')} />
          <Text style={styles.sensorValue}>{node.temperature}°C</Text>
        </View>
        <View style={{width: '100%', flex: 1, flexDirection: 'row'}}>
          { !node.humidity? null :
            <>
              <Image style={{width: 30, height: 30, resizeMode: 'contain'}} source={require('./icons/hum-icon.png')} />
              <Text style={styles.sensorValue}>{node.humidity}%</Text>
            </>
          }
        </View>
      </View>
    </View>
  );
};

const styles = StyleSheet.create({
  item: {
    width: '100%',
    flex: 1,
    borderBottomColor: '#88A',
    borderBottomWidth: 1,
  },
  sensorHeaderText: {
    fontSize: 16,
    color: '#118',
  },
  sensorHeaderTextError: {
    fontSize: 16,
    color: '#A00',
  },
  sensorValue: {
    fontSize: 24,
    color: '#000',
    marginBottom: 3,
    width: '50%'
  }
});

const dateTimeUpd = (date: Date, now: Date) => {
  const diff = Math.round((now.getTime() - date.getTime()) / 1000);
  if(diff < 24 * 3600) {
    if(diff < 3600) {
      if(diff < 60) {
        if(diff < 10) {
          return `just now`;
        }
        return `${diff}s ago`;
      }
      return `${Math.round(diff/60)}m ago`;
    }
    return `${Math.round(diff/3600)}h ago`;
  }
  return format(date, "dd.MM.yy HH:mm");
}

const UpdDate = ({date}: {date: Date}) => {
  const [now, setNow] = React.useState(new Date());
  React.useEffect(() => {
    setNow(new Date());
    const iid = setInterval(() => setNow(new Date()), 10000);
    return () => clearInterval(iid);
  }, [date]);
  return (<>{ dateTimeUpd(date, now) }</>);
}

const BatteryIcon = ({batteryLevel} : {batteryLevel: WeatherNode['batteryLevel']}) => {
  if(!batteryLevel) return null;
  const img = 
    batteryLevel == 'low'? require('./icons/bat-0.png') :
    batteryLevel == 'medium-low'? require('./icons/bat-1.png') :
    batteryLevel == 'medium-high'? require('./icons/bat-2.png') :
    require('./icons/bat-3.png');
  return <Image style={{width: 20, height: 20, resizeMode: 'contain'}} source={img} />;
};
