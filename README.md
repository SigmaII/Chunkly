Rivus (dal latino "fiume") è un progetto nato con il desiderio di scrivere uno strumento in grado di trasferire file in maniera veloce, chunk-based ed in maniera
affidabile.

Utilizza un protocollo L7 scritto in C per la condivisione dei file. Di seguito di riporta la struttura generale:

                                              
| Field                  | Size (bytes) | Description                |
|------------------------|--------------|----------------------------|
| filename length        | 4            | filename length (big endian) |
| filename               | N            | filename                   |
| file size              | 8            | file size (big endian)     |
| payload size           | 8            | payload size (big endian)  |
| payload                | M            | payload                    |

                                              
