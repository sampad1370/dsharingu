#!/bin/bash

	/bin/rm screenshots/*.png
	pushd	screenshots_src
	i=0
	for f in *
	do
		let i+=1
		fext=${f##*.}

		/bin/cp "$f" $(printf "../screenshots/ds_sshot_%02i.$fext" $i)

		convert -resize 200x200 -quality 85 +profile "*" "$f" \
			$(printf "../screenshots/ds_sshot_%02i_ico.$fext" $i)
	done
	popd
