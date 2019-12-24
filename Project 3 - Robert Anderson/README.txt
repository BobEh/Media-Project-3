This program works in Debug Win32 mode in visual studio.

This program shows a scene (using only audio) of a man who was at a dinner party and then leaves. On his way home he is pulled over by the police but thankfully they only gave him a warning and no ticket and he continues on his way home.

To change the sound clip selections:
	1. Access them in SolutionDir/../common/assets/audio
	1.5. Access compressed audio files in SolutionDir/../common/assets/audio/compressed
	2. Upload any file you would like to use.
	3. Access the .txt document in the SolutionDir (soundLibrary.txt for uncompressed and soundLibraryCompressed.txt for compressed files).
	4. Update/Add to the paths to include the newly uploaded files.

To run the program:
	- When the program starts up it will ask if you would like to use the compressed or un-compressed audio files. Click 'c' for compressed and 'u' for uncompressed and then press enter.
	- Once the program starts you will have the following controls for the audio:
	   - Press ESC to Exit.
	   - Press SPACE to pause and play.
	   - Press TAB to go to the next audio clip.
	   - Press SHIFT+TAB to go to the previous audio clip.
	   - Press UP or DOWN to control the volume.
	   - Press LEFT and RIGHT to control the pan.