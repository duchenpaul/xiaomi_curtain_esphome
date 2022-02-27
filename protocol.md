## Close
```
down set_properties 2 1 2\r:result 2 1 0
```
| txd                       | rxd                                |
| ------------------------- | ---------------------------------- |
| down set_properties 2 1 2 | result 2 1 0                       |
| ok                        | properties_changed 2 2 100 2 3 100 |
| ok                        |                                    |

## Pause
| txd                       | rxd                                |
| ------------------------- | ---------------------------------- |
| down set_properties 2 1 0 | result 2 1 0                       |
| ok                        | properties_changed 2 2 100 2 3 100 |
| ok                        |                                    |


## Open
```
down set_properties 2 1 1\r:result 2 1 0
```

## slide
```
down set_properties 2 3 45\r:properties_changed 2 1 0/result 2 3 0
```


## reverse-0
```
down set_properties 2 4 0\r:properties_changed 2 4 0/result 2 4 0
```

## reverse-1 *
```
down set_properties 2 4 1:properties_changed 2 4 1/result 2 4 0
```

## heartbeat
```
down none:get_down
```


## calib
```
down set_properties 2 4 2:result 2 4 0
```


## get status
down get_properties 2 3:result 2 3 0 100
