
#!/bin/zsh
xcrun simctl shutdown 72C8208F-E181-4AA4-B9CC-036B379BA1E7
pkill -9 Simulator
xcrun simctl boot 72C8208F-E181-4AA4-B9CC-036B379BA1E7
open /Applications/Xcode-beta.app/Contents/Developer/Applications/Simulator.app
#lldb -o run /Applications/Xcode-beta.app/Contents/Developer/Applications/Simulator.app/Contents/MacOS/Simulator
#/Applications/Xcode-beta.app/Contents/Developer/Applications/Simulator.app/Contents/MacOS/Simulator