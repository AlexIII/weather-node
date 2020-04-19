/*
 * Weather Node Station
 * author: github.com/AlexIII
 * e-mail: endoftheworld@bk.ru
 * license: MIT
 */

import React from 'react';
import { StyleSheet, Text, View, Modal} from 'react-native';
import { Touchable } from './Touchable';

export interface DialogProps {
  buttons: Array<{
    title: string;
    onPress: () => void;
  }>;
}

export const Dialog = ({ buttons, children }: React.PropsWithChildren<DialogProps>) => {
  if(buttons.length < 1) throw Error('Dialog component must have at least one button.');
  return (
    <Modal animationType='slide' transparent={true} >
      <View style={styles.centeredView}>
        <View style={styles.modalView}>
          { children }
          <View style={styles.buttonGroup}>
            {
              buttons.map((button, idx) => (
                <Touchable onPress={button.onPress} key={idx}>
                  <View style={styles.button} >
                    <Text style={styles.textStyle}>{ button.title }</Text>
                  </View>
                </Touchable>  
              ))
            }
          </View>
        </View>
      </View>
    </Modal>
  );
};

const styles = StyleSheet.create({
  centeredView: {
    flex: 1,
    justifyContent: "center",
    alignItems: "center",
    marginTop: 22
  },
  modalView: {
    width: '80%',
    backgroundColor: "white",
    borderRadius: 10,
    padding: 15,
    alignItems: "center",
    shadowColor: "#000",
    shadowOffset: {
      width: 0,
      height: 2
    },
    shadowOpacity: 0.25,
    shadowRadius: 3.84,
    elevation: 5
  },
  button: {
    backgroundColor: "#999",
    borderRadius: 6,
    padding: 10,
    elevation: 2,
    width: 100,
    margin: 5
  },
  textStyle: {
    color: "white",
    fontWeight: "bold",
    textAlign: "center"
  },
  buttonGroup: {
    alignItems: "center",
    flexDirection: 'row',
    justifyContent: 'space-around'
  }
});