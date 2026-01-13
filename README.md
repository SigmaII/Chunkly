Rivus (dal latino "fiume") è un progetto nato con il desiderio di scrivere uno strumento in grado di trasferire file in maniera veloce, chunk-based ed in maniera
affidabile.

Utilizza un protocollo L7 scritto in C per la condivisione dei file. Di seguito di riporta la struttura generale:

                                              |
 [4 byte ]  filename length (big endian)      |
 [N byte ]  filename                          |
 [8 byte]   file size (big endian)            |
 [8 byte ]  payload size (big endian)         |
 [M byte ]  payload                           |
                                              |
