/*
 * Weather Node Station
 * author: github.com/AlexIII
 * e-mail: endoftheworld@bk.ru
 * license: MIT
 */

import React from 'react';
import { StyleSheet, Text, TextInput} from 'react-native';
import { Dialog } from './Dialog';

export interface SensorNameFormProps {
  mac: string;
  name: string;
  onChange: (name: string) => void;
}

export const SensorNameForm = ({mac, name, onChange}: SensorNameFormProps) => {
  const [newName, setNewName] = React.useState(name);
  const onCancel = React.useCallback(() => onChange(name), []);
  const onOk = React.useCallback(() => onChange(newName), [newName]);

  return (
    <Dialog buttons={[{title: 'CANCEL', onPress: onCancel}, {title: 'OK', onPress: onOk}]}>
      <Text style={styles.modalText}>New name for {mac}</Text>
      <TextInput style={styles.textInput} value={newName} onChangeText={setNewName} selectTextOnFocus />
    </Dialog>
  );
};

const styles = StyleSheet.create({
  modalText: {
    marginBottom: 15,
    textAlign: "center"
  },
  textInput: {
    height: 40, 
    borderColor: '#AAA', 
    borderRadius: 6,
    width: '100%',
    borderWidth: 1
  }
});