# juce_meets_ndi
 
This project is an example of integrating NewTek™ NDI with the JUCE framework.
Under the power of the JUCE framework, this software can be built as a VST3 plugin.

This project offers a few key features:  
- NdiSender can run on the DAW and send video and audio as an NDI signal.
- NdiReceiver can run on the DAW and receive video and audio as an NDI signal.
 
## How to build

In advance, download the NDI SDK from NewTek's official website here: https://www.ndi.tv/sdk/
And, execute install for your build environment.
Please note that in order to use the NDI SDK, you must agree to the NDI SDK license agreement.

### Windows

```
$ git clone https://github.com/COx2/juce_meets_ndi.git
$ git submodule update --init --recursive
$ .\NdiSender\build_msvc2019.bat
$ .\NdiReceiver\build_msvc2019.bat
```

### macOS

```
$ git clone https://github.com/COx2/juce_meets_ndi.git
$ git submodule update --init --recursive
$ ./NdiSender/build_xcode.command
$ ./NdiReceiver/build_xcode.command
```

## Install instructions

### Windows

Install the NDI V4 Windows runtime using the installer provided here: http://new.tk/NDIRedistV4
On Windows, you must reboot your computer to make a new or updated NDI Runtime installation effective

### macOS

Install the NDI V4 macOS runtime using the installer provided here: http://new.tk/NDIRedistV4Apple


## Contributing
 
1. Fork it!
2. Create your feature branch: `git checkout -b my-new-feature`
3. Commit your changes: `git commit -am 'Add some feature'`
4. Push to the branch: `git push origin my-new-feature`
5. Submit a pull request :D
 
## History
 
Version 0.0.1 (2020-06-26) - For proof of concept
 
## Credits

Company name - Shoegaze Systems
Developer - Tatsuya Shiozawa (@COx2)
 
## License

MIT License

Copyright (c) 2020 Tatsuya Shiozawa

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
