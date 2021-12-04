#!/bin/bash



# -- reads the file formats to be ignored from given text file and stores them into an array
collect_ignored_formats()
{
	## -- reading from file and storing into array -- ##
	idx=0
	while read line; do
		idx=$((idx+1))
		formats_to_be_ignored[idx]=$line								# putting the lines in an array
	done < $file
	echo Formats To Be Ignored: ${formats_to_be_ignored[*]}
}


# -- returns the file extension ; param -> filename
get_file_extension()
{
	filename=`echo $1|xargs`											# trimming all the whitespaces
	res="${filename//[^.]}"												# getting the count of what's after dot

	if [ ${#res} -eq 0 ]; then											# if nothing is after dot i.e. no file extension given
		extension="others"												# then extension is 'others'
	else
		extension="${filename##*.}"										# getting the rest after the last occurrence of .
	fi
}


# -- collects all the selected file formats ; params -> directory, level
collect_selected_formats()
{
	cd "$1"															# go to the directory
	level=$2														# directory level from given directory

	for f in *
	do
		if [ -f "$f" ]; then										# if selected item is a file
			
			get_file_extension $f 									# getting the file extension
			
			if [[ ! "$f" = "$file_actual_name" ]]; then												# if file is not "input.txt"
				if [[ ! " ${formats_to_be_ignored[*]} " =~ " $extension " ]]; then					# if file is not in format_to_ignored
					if [[ ! " ${selected_formats[*]} " =~ " $extension " ]]; then					# if format is not already included
						selected_formats+=($extension)												# appending to the selected_list
					fi
				else
					ignored_file_count=$((ignored_file_count + 1))									# if file is to be ignored, increase the count
				fi
			fi

		elif [ -d "$f" ]; then											# if selected item is a directory
			new_level=$((level+1))										# increase the level
			collect_selected_formats "$f" $new_level					# call recursively
		fi
	done

	cd ../																# come back
}


# -- collects all the files in an array except the files to be ignored
move_selected_files()
{
	cd "$1"
	level=$2

	for f in *
	do
		if [ -f "$f" ]; then																# if selected item is a file
			
			get_file_extension "$f" 															# getting the file extension
			
			if [[ ! "$f" = "$file_actual_name" ]]; then											# if file is not "input.txt"
				if [[ ! " ${formats_to_be_ignored[*]} " =~ " $extension " ]]; then				# if this format is not to be ignored

					if [[ ! "$f" = "$file_actual_name" ]]; then												
						mv "$f" ${output_dir_path}output_dir/$extension 						# move the file to the sub-directory
					fi
					
					abs_path=$( realpath "$f" )													# getting the absolute path of the file
					x=${dir##*./}						
					relative_path="${abs_path##*$x}"											# trimming everything before working_dir										
					echo $relative_path>>${output_dir_path}output_dir/$extension/desc.txt		# appending the file path to the desc file
					# relative_path=$( realpath --relative-to=. "$f" )							# getting the relative path of the file

				fi
			fi

		elif [ -d "$f" ]; then															# if selected item is a directory
			new_level=$((level+1))														# increase the level
			move_selected_files "$f" $new_level											# call recursively
		fi
	done

	cd ../
}


# -- make directories of a formats
make_directories()
{
	rm -r "${output_dir}output_dir"												# removing the directory if it already exists
	mkdir -p "${output_dir}output_dir"											# create an output directory
	touch ${output_dir}output.csv									# create the csv file in output_directory
	echo "Format, No_of_files">>${output_dir_path}output.csv					# creating two columns in the csv file

	for i in ${!selected_formats[@]}											# for each file in selected formats
	do
		mkdir -p ${output_dir_path}output_dir//${selected_formats[$i]}			# create sub-directories for different formats
		touch ${output_dir_path}output_dir/${selected_formats[$i]}/desc.txt		# creating a description text file in the sub-directory							
	done
}


# -- count the files of each format and put them into csv files
populate_csv_file()
{
	for f in ${output_dir}output_dir/*
	do
		if [[ -d "$f" ]]; then																	# for every directory in output_dir
			format_name=${f##*/}																# taking the folder name aka format name 
			count=$(ls $f|wc -l)																# taking the file count
			echo $format_name, $((count-1))>>${output_dir_path}output.csv						# entering value; count-1 because of the desc.txt file 
		fi
	done
	echo "ignored", $ignored_file_count>>${output_dir_path}output.csv 							# entering the ignored cases
}


# -- main process -- #
if [ $# -eq 2 ]; then														# if 2 params are given i.e. directory and file
	dir=$1
	file=$2
	file_actual_name="$(basename "$file")"

	if [ -d "$dir" ]; then													# if directory exists
		
		if [ -f "$file" ]; then												# if file exists
			echo "All inputs valid. Process starting..."
			
			selected_formats=()												# declaring the array that will contain all the selected formats
			ignored_file_count=0
			output_dir_path=`realpath .`/									# e.g. ~/Work/offline/

			collect_ignored_formats											
			collect_selected_formats $dir 1
			make_directories
			move_selected_files $dir 1
			populate_csv_file

			echo Selected formats: ${selected_formats[*]}

		else										
			echo "$file does not exist. Please provide a valid file."
			echo "Please run <./script.sh> <your_directory> <input text file>" 
		fi
	
	else
		echo "Directory $dir does not exist."
	fi

elif [ $# -eq 1 ]; then														# if only 1 param is given -> i.e. fileName
	dir=$(pwd)																# then root is the working directory														
	file=`realpath $1`
	file_actual_name="$(basename "$file")"

	if [ -d "$dir" ] && [ -f "$file" ]; then								# if directory and file is valid
			echo "All inputs valid. Process starting..."

			cd ..															# going to the parent directory
			
			selected_formats=()												# declaring the array that will contain all the selected formats
			ignored_file_count=0
			output_dir_path=`realpath .`/									# ~/Work/offline/												
			

			collect_ignored_formats											
			collect_selected_formats $dir 1
			make_directories
			move_selected_files $dir 1
			populate_csv_file

			echo Selected formats: ${selected_formats[*]}


	else
		echo "$file does not exist. Please provide a valid file."
		echo "Please run <./script.sh> <your_directory> <input text file>" 
	fi

else
	echo "Please Provide a Directory and an Input File"
fi