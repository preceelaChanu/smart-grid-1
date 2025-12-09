#!/usr/bin/env python3
"""
Advanced HHBlock Data Converter with Progress Tracking and Error Handling

This script provides multiple ways to convert HHBlock dataset files:
1. Convert single files
2. Convert all files in a directory
3. Merge multiple converted files into one large dataset
4. Validate converted data
"""

import pandas as pd
import numpy as np
from datetime import datetime, timedelta
import os
import glob
from pathlib import Path
import argparse
from tqdm import tqdm
import logging

# Set up logging
logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s')
logger = logging.getLogger(__name__)

class HHBlockConverter:
    def __init__(self, input_dir, output_dir):
        self.input_dir = Path(input_dir)
        self.output_dir = Path(output_dir)
        self.output_dir.mkdir(parents=True, exist_ok=True)
        
    def convert_single_file(self, input_file, output_file=None, show_progress=True):
        """Convert a single HHBlock CSV file to normalized format."""
        input_path = Path(input_file)
        
        if output_file is None:
            output_file = self.output_dir / f"{input_path.stem}_converted.csv"
        
        try:
            # Read the input file
            df = pd.read_csv(input_path)
            logger.info(f"Processing {input_path.name} with {len(df)} days of data")
            
            # Prepare data for conversion
            converted_data = []
            
            # Process with progress bar if requested
            iterator = tqdm(df.iterrows(), total=len(df), desc=f"Converting {input_path.name}") if show_progress else df.iterrows()
            
            for _, row in iterator:
                meter_id = row['LCLid']
                date_str = row['day']
                
                try:
                    # Parse the date
                    date = pd.to_datetime(date_str)
                    
                    # Process each half-hourly value
                    for hh_idx in range(48):
                        hh_col = f'hh_{hh_idx}'
                        
                        if hh_col in row and pd.notna(row[hh_col]) and row[hh_col] != '':
                            consumption = float(row[hh_col])
                            
                            # Calculate timestamp (hh_0 = 00:00, hh_1 = 00:30, etc.)
                            timestamp = date + timedelta(minutes=hh_idx * 30)
                            
                            converted_data.append({
                                'meter_id': meter_id,
                                'timestamp': timestamp,
                                'consumption_kwh': consumption
                            })
                            
                except Exception as e:
                    logger.warning(f"Error processing row for {meter_id} on {date_str}: {e}")
                    continue
            
            if converted_data:
                # Create DataFrame and save
                result_df = pd.DataFrame(converted_data)
                result_df['timestamp'] = pd.to_datetime(result_df['timestamp'])
                result_df = result_df.sort_values(['meter_id', 'timestamp'])
                
                result_df.to_csv(output_file, index=False)
                logger.info(f"Successfully converted {len(converted_data)} readings to {output_file}")
                return len(converted_data)
            else:
                logger.warning(f"No valid data found in {input_path}")
                return 0
                
        except Exception as e:
            logger.error(f"Failed to process {input_path}: {e}")
            return 0
    
    def convert_all_files(self, file_pattern="block_*.csv"):
        """Convert all matching files in the input directory."""
        pattern = self.input_dir / file_pattern
        input_files = list(glob.glob(str(pattern)))
        
        if not input_files:
            logger.error(f"No files matching pattern '{file_pattern}' found in {self.input_dir}")
            return
        
        input_files.sort()
        logger.info(f"Found {len(input_files)} files to convert")
        
        total_records = 0
        successful_files = 0
        
        for input_file in input_files:
            try:
                records = self.convert_single_file(input_file, show_progress=True)
                if records > 0:
                    total_records += records
                    successful_files += 1
            except Exception as e:
                logger.error(f"Failed to convert {input_file}: {e}")
        
        logger.info(f"Conversion complete! Successfully converted {successful_files}/{len(input_files)} files")
        logger.info(f"Total records converted: {total_records:,}")
    
    def merge_converted_files(self, output_file="merged_consumption_data.csv", pattern="*_converted.csv"):
        """Merge all converted files into a single dataset."""
        pattern_path = self.output_dir / pattern
        converted_files = list(glob.glob(str(pattern_path)))
        
        if not converted_files:
            logger.error(f"No converted files found with pattern '{pattern}' in {self.output_dir}")
            return
        
        logger.info(f"Merging {len(converted_files)} converted files...")
        
        # Read and combine all files
        dataframes = []
        for file_path in tqdm(converted_files, desc="Reading files"):
            try:
                df = pd.read_csv(file_path)
                df['timestamp'] = pd.to_datetime(df['timestamp'])
                dataframes.append(df)
            except Exception as e:
                logger.warning(f"Error reading {file_path}: {e}")
        
        if dataframes:
            # Combine all dataframes
            merged_df = pd.concat(dataframes, ignore_index=True)
            merged_df = merged_df.sort_values(['meter_id', 'timestamp'])
            
            # Save merged file
            output_path = self.output_dir / output_file
            merged_df.to_csv(output_path, index=False)
            
            logger.info(f"Merged dataset saved to {output_path}")
            logger.info(f"Total records: {len(merged_df):,}")
            logger.info(f"Date range: {merged_df['timestamp'].min()} to {merged_df['timestamp'].max()}")
            logger.info(f"Unique meters: {merged_df['meter_id'].nunique()}")
    
    def validate_converted_data(self, file_path):
        """Validate a converted data file."""
        try:
            df = pd.read_csv(file_path)
            
            print(f"\nValidation Report for {Path(file_path).name}")
            print("=" * 50)
            print(f"Total records: {len(df):,}")
            print(f"Unique meters: {df['meter_id'].nunique()}")
            print(f"Date range: {df['timestamp'].min()} to {df['timestamp'].max()}")
            print(f"Consumption stats (kWh):")
            print(f"  Min: {df['consumption_kwh'].min():.6f}")
            print(f"  Max: {df['consumption_kwh'].max():.6f}")
            print(f"  Mean: {df['consumption_kwh'].mean():.6f}")
            print(f"  Std: {df['consumption_kwh'].std():.6f}")
            
            # Check for missing values
            missing_values = df.isnull().sum()
            if missing_values.sum() > 0:
                print(f"\nMissing values:")
                for col, count in missing_values.items():
                    if count > 0:
                        print(f"  {col}: {count}")
            else:
                print("\nNo missing values found ✓")
            
            # Check for duplicate timestamps per meter
            duplicates = df.groupby('meter_id')['timestamp'].apply(lambda x: x.duplicated().sum()).sum()
            if duplicates > 0:
                print(f"\nDuplicate timestamps found: {duplicates}")
            else:
                print("No duplicate timestamps found ✓")
                
        except Exception as e:
            logger.error(f"Error validating {file_path}: {e}")

def main():
    parser = argparse.ArgumentParser(description="Convert HHBlock dataset to normalized format")
    parser.add_argument("--input-dir", default=r"D:\london\hhblock_dataset\hhblock_dataset",
                      help="Input directory containing HHBlock CSV files")
    parser.add_argument("--output-dir", default=r"D:\london\converted_data",
                      help="Output directory for converted files")
    parser.add_argument("--single-file", help="Convert a single file")
    parser.add_argument("--merge", action="store_true", help="Merge all converted files")
    parser.add_argument("--validate", help="Validate a converted data file")
    
    args = parser.parse_args()
    
    converter = HHBlockConverter(args.input_dir, args.output_dir)
    
    if args.single_file:
        converter.convert_single_file(args.single_file)
    elif args.merge:
        converter.merge_converted_files()
    elif args.validate:
        converter.validate_converted_data(args.validate)
    else:
        # Convert all files by default
        converter.convert_all_files()

if __name__ == "__main__":
    main()