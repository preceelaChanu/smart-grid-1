#!/usr/bin/env python3
"""
Proper Meter Data Separator

This script correctly separates meter data from converted CSV files into individual files per meter.
It handles the case where converted files contain data for multiple meters mixed together.
"""

import pandas as pd
import os
import glob
from pathlib import Path
from collections import defaultdict
import logging
from tqdm import tqdm

# Set up logging
logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s')
logger = logging.getLogger(__name__)

class ProperMeterSeparator:
    def __init__(self, converted_data_dir, meter_output_dir):
        self.converted_data_dir = Path(converted_data_dir)
        self.meter_output_dir = Path(meter_output_dir)
        self.meter_output_dir.mkdir(parents=True, exist_ok=True)
        
        # Dictionary to store meter data temporarily
        self.meter_data = defaultdict(list)
        
    def process_all_converted_files(self):
        """Process all converted files and separate meters correctly"""
        
        # Find all converted CSV files
        converted_files = list(self.converted_data_dir.glob("*_converted.csv"))
        
        if not converted_files:
            logger.error(f"No converted files found in {self.converted_data_dir}")
            return
        
        logger.info(f"Found {len(converted_files)} converted files to process")
        
        # Step 1: Collect all data and group by meter_id
        logger.info("Step 1: Reading and grouping data by meter ID...")
        
        for file_path in tqdm(converted_files, desc="Reading files"):
            try:
                df = pd.read_csv(file_path)
                
                # Group by meter_id and add to our collection
                for meter_id in df['meter_id'].unique():
                    meter_df = df[df['meter_id'] == meter_id].copy()
                    self.meter_data[meter_id].append(meter_df)
                    
            except Exception as e:
                logger.error(f"Error processing {file_path}: {e}")
        
        logger.info(f"Found data for {len(self.meter_data)} unique meters")
        
        # Step 2: Combine and save data for each meter
        logger.info("Step 2: Combining and saving meter files...")
        
        for meter_id in tqdm(self.meter_data.keys(), desc="Saving meter files"):
            try:
                # Combine all dataframes for this meter
                meter_df = pd.concat(self.meter_data[meter_id], ignore_index=True)
                
                # Sort by timestamp
                meter_df['timestamp'] = pd.to_datetime(meter_df['timestamp'])
                meter_df = meter_df.sort_values('timestamp')
                
                # Remove duplicates (if any)
                initial_count = len(meter_df)
                meter_df = meter_df.drop_duplicates(['meter_id', 'timestamp'])
                final_count = len(meter_df)
                
                if initial_count != final_count:
                    logger.info(f"Removed {initial_count - final_count} duplicate records for {meter_id}")
                
                # Save to individual meter file
                output_file = self.meter_output_dir / f"meter_{meter_id}.csv"
                meter_df.to_csv(output_file, index=False)
                
                logger.info(f"Saved {len(meter_df):,} records for meter {meter_id}")
                
            except Exception as e:
                logger.error(f"Error saving data for meter {meter_id}: {e}")
        
        # Clear memory
        self.meter_data.clear()
        
        logger.info("Meter separation completed successfully!")
    
    def validate_meter_files(self):
        """Validate the separated meter files"""
        meter_files = list(self.meter_output_dir.glob("meter_*.csv"))
        
        if not meter_files:
            logger.error("No meter files found to validate")
            return
        
        logger.info(f"Validating {len(meter_files)} meter files...")
        
        total_records = 0
        validation_results = []
        
        for meter_file in tqdm(meter_files, desc="Validating"):
            try:
                df = pd.read_csv(meter_file)
                meter_id = df['meter_id'].iloc[0]
                
                # Check that all records are for the same meter
                unique_meters = df['meter_id'].unique()
                if len(unique_meters) != 1:
                    logger.warning(f"File {meter_file.name} contains multiple meters: {unique_meters}")
                
                # Get date range
                df['timestamp'] = pd.to_datetime(df['timestamp'])
                date_range = f"{df['timestamp'].min()} to {df['timestamp'].max()}"
                
                validation_results.append({
                    'meter_id': meter_id,
                    'records': len(df),
                    'date_range': date_range,
                    'file': meter_file.name
                })
                
                total_records += len(df)
                
            except Exception as e:
                logger.error(f"Error validating {meter_file}: {e}")
        
        # Print summary
        print(f"\nMeter Files Validation Summary")
        print("=" * 60)
        print(f"Total meter files: {len(meter_files)}")
        print(f"Total records: {total_records:,}")
        print()
        print("Sample of meter files:")
        for result in validation_results[:10]:  # Show first 10
            print(f"  {result['meter_id']}: {result['records']:,} records ({result['date_range']})")
        
        if len(validation_results) > 10:
            print(f"  ... and {len(validation_results) - 10} more meter files")
        
        return validation_results

def main():
    """Main function to run the proper meter separation"""
    
    converted_data_dir = r"D:\london\preproprocessed\converted_data"
    meter_output_dir = r"D:\london\preproprocessed\converted_data_c"
    
    print("Proper Meter Data Separator")
    print("=" * 50)
    print(f"Input directory: {converted_data_dir}")
    print(f"Output directory: {meter_output_dir}")
    print()
    
    separator = ProperMeterSeparator(converted_data_dir, meter_output_dir)
    
    # Process all files
    separator.process_all_converted_files()
    
    # Validate results
    print()
    separator.validate_meter_files()

if __name__ == "__main__":
    main()