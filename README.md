# termux-usb-examples

termux-usb を用いて、以下を実装しました。

1. 1200bps でポートを開く (Bootloader に遷移)
2. USB CDC を用いて最低限のシリアル通信表示を行う
3. 書き込みを行う

## 使い方

Makefile に記載の使い方ができます。

最初に DEVICE_PATH を取得します。
以下でいう所の `/dev/bus/usb/002/005` です。

```
$ termux-usb -l
[
  "/dev/bus/usb/002/005"
]
```

続いて、 `make req` で権限を取得します。

```
$ make DEVICE_PATH=/dev/bus/usb/002/005 req
termus-usb -r /dev/bus/usb/002/005
Access granted.
```

`make touch` でブートローダーに遷移させます。

```
$ make DEVICE_PATH=/dev/bus/usb/002/005 touch
termux-usb -e ./u-touch /dev/bus/usb/002/005
Vendor ID: 2886
Product ID: 802d
Manufacturer: Seeed
Product: Seeed Wio Terminal
mode: touch (2)
```

DEVICE_PATH が変わるので再度取得します。

```
$ termux-usb -l
[
  "/dev/bus/usb/002/006"
]
```


`make` で書き込みます。
書き込みに使用する uf2 ファイルは main.c 内に直接記載されています。

* https://github.com/sago35/termux-usb-examples/blob/30285de61cc03e66780d56b64474e9f923ffc638/main.c#L343-L344

```
$ make DEVICE_PATH=/dev/bus/usb/002/006
termux-usb -e ./usbtest /dev/bus/usb/002/005
(省略)
size 15360
-- fopen
-- f_write
-- f_close
```

## u-touch および u-monitor のビルド

`make usbtest` を実行して `u-touch` および `u-monitor` にリネームしてください。
`make usbtest` 実行時に以下の MODE を 1 にすると `u-monitor` 用に、 2 にすると `u-touch` 用になります。
書き込みに使用するときは MODE を 0 にする必要があります。

https://github.com/sago35/termux-usb-examples/blob/30285de61cc03e66780d56b64474e9f923ffc638/main.c#L17-L22
