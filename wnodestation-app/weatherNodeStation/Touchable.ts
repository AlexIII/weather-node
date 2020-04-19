/*
 * Weather Node Station
 * author: github.com/AlexIII
 * e-mail: endoftheworld@bk.ru
 * license: MIT
 */

import { TouchableNativeFeedback, TouchableOpacity, Platform } from 'react-native';
export const Touchable = (
        Platform.OS === 'android' ? TouchableNativeFeedback : TouchableOpacity
    ) as unknown as React.ComponentType<{onPress: () => void}>;
