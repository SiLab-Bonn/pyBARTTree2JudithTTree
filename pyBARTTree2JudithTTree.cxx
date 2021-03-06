/* Converter from pyBAR TTree to Judith TTree.
 * Compatible with CERN ROOT 5 and 6.
 */

#include <iostream>
#include <math.h>

#include <TFile.h>
#include <TTree.h>
#include <TBranch.h>
#include <TDirectory.h>


int pyBARTTree2JudithTTree(const char* input_file_name, const char* output_file_name, const char* plane = "Plane0", const char* mode = "RECREATE", bool fill_event = true, Long64_t max_events = 0, bool check_timestamp = true) {
	// Call with: root -l -b -q "pyBARTTree2JudithTTree.cxx(\"input.root\", \"output.root\", \"Plane0\", \"RECREATE\", 1, 0))"
	// To add another plane: root -l -b -q "pyBARTTree2JudithTTree.cxx(\"input.root\", \"output.root\", \"Plane1\", \"UPDATE\", 0, 0))"

	// the buffer size depends on the input file
	static const Long64_t BUF_SIZE = 100000;
	static const Int_t MAX_HITS = 4000;

	// pyBAR ROOT file
	TFile* i_file = new TFile(input_file_name, "READ");
	TTree* pybar_ttree = (TTree*) i_file->Get("Hits");

	// Judith ROOT file
	TFile* j_file = new TFile(output_file_name, mode);
	TTree* event_ttree;
	if (fill_event) {
		event_ttree = (TTree*) j_file->Get("Event");
		if (event_ttree != 0) {
			std::cout << "Event TTree already exists" << std::endl;
			throw;
		}
		event_ttree = new TTree("Event", "Event");
	} else {
		event_ttree = (TTree*) j_file->Get("Event");
		if (event_ttree == 0) {
			std::cout << "Event TTree not existing" << std::endl;
			throw;
		}
	}
	TDirectory* dir = j_file->mkdir(plane);
	if (dir!=0) {
	    dir->cd();
	} else {
		std::cout << "plane already exists" << std::endl;
		throw;
		//j_file->cd(plane);
	}
	TTree* hits_ttree = new TTree("Hits", "Hits");

	// event number and counter
	Long64_t event_counter = 0;
	Long64_t curr_event_number = -1; // set to invalid event number
	bool reached_max_events = false;

	// timestamp
	ULong64_t judith_last_time_stamp;
	ULong64_t judith_delta_time_stamp;
	ULong64_t current_time_stamp;
	ULong64_t last_time_stamp;
	ULong64_t delta_time_stamp;
	Double_t time_stamp_ratio;

	// pyBAR arrays
	Long64_t pybar_n_entries;
	Long64_t pybar_event_number[BUF_SIZE];
	UInt_t pybar_trigger_number[BUF_SIZE];
	UInt_t pybar_trigger_time_stamp[BUF_SIZE];
	UChar_t pybar_relative_bcid[BUF_SIZE];
	UShort_t pybar_lvl1id[BUF_SIZE];
	UChar_t pybar_column[BUF_SIZE];
	UShort_t pybar_row[BUF_SIZE];
	UChar_t pybar_tot[BUF_SIZE];
	UShort_t pybar_bcid[BUF_SIZE];
	UShort_t pybar_tdc[BUF_SIZE];
	UShort_t pybar_tdc_time_stamp[BUF_SIZE];
	UChar_t pybar_trigger_status[BUF_SIZE];
	UInt_t pybar_service_record[BUF_SIZE];
	UShort_t pybar_event_status[BUF_SIZE];

	// Judith arrays
	Int_t judith_n_hits = 0;
	Int_t judith_hit_pix_x[MAX_HITS];
	Int_t judith_hit_pix_y[MAX_HITS];
	Double_t judith_hit_pos_x[MAX_HITS];
	Double_t judith_hit_pos_y[MAX_HITS];
	Double_t judith_hit_pos_z[MAX_HITS];
	Int_t judith_hit_value[MAX_HITS];
	Int_t judith_hit_timing[MAX_HITS];
	Int_t judith_hit_in_cluster[MAX_HITS];
	UShort_t judith_tdc[MAX_HITS];
	UShort_t judith_tdc_time_stamp[MAX_HITS];
	UChar_t judith_trigger_status[MAX_HITS];
	UShort_t judith_event_status[MAX_HITS];


	// Event arrays
	ULong64_t judith_time_stamp;
	ULong64_t judith_frame_number;
	Int_t judith_trigger_offset;
	Int_t judith_trigger_info;
	Bool_t judith_invalid;

	// pyBAR branches
	pybar_ttree->SetBranchAddress("n_entries", &pybar_n_entries);
	pybar_ttree->SetBranchAddress("event_number", pybar_event_number);
	pybar_ttree->SetBranchAddress("trigger_number", pybar_trigger_number);
	pybar_ttree->SetBranchAddress("trigger_time_stamp", pybar_trigger_time_stamp);
	pybar_ttree->SetBranchAddress("relative_BCID", pybar_relative_bcid);
	pybar_ttree->SetBranchAddress("LVL1ID", pybar_lvl1id);
	pybar_ttree->SetBranchAddress("column", pybar_column);
	pybar_ttree->SetBranchAddress("row", pybar_row);
	pybar_ttree->SetBranchAddress("tot", pybar_tot);
	pybar_ttree->SetBranchAddress("BCID", pybar_bcid);
	pybar_ttree->SetBranchAddress("TDC", pybar_tdc);
	pybar_ttree->SetBranchAddress("TDC_time_stamp", pybar_tdc_time_stamp);
	pybar_ttree->SetBranchAddress("trigger_status", pybar_trigger_status);
	pybar_ttree->SetBranchAddress("service_record", pybar_service_record);
	pybar_ttree->SetBranchAddress("event_status", pybar_event_status);

	// Judith branches
	hits_ttree->Branch("NHits", &judith_n_hits, "NHits/I");
	hits_ttree->Branch("PixX", judith_hit_pix_x, "HitPixX[NHits]/I");
	hits_ttree->Branch("PixY", judith_hit_pix_y, "HitPixY[NHits]/I");
	hits_ttree->Branch("Value", judith_hit_value, "HitValue[NHits]/I");
	hits_ttree->Branch("Timing", judith_hit_timing, "HitTiming[NHits]/I");
	hits_ttree->Branch("InCluster", judith_hit_in_cluster, "HitInCluster[NHits]/I");
	hits_ttree->Branch("PosX", judith_hit_pos_x, "HitPosX[NHits]/D");
	hits_ttree->Branch("PosY", judith_hit_pos_y, "HitPosY[NHits]/D");
	hits_ttree->Branch("PosZ", judith_hit_pos_z, "HitPosZ[NHits]/D");
	hits_ttree->Branch("Tdc", judith_tdc, "HitTdc[NHits]/s");
	hits_ttree->Branch("TdcTs", judith_tdc_time_stamp, "HitTdcTs[NHits]/s");
	hits_ttree->Branch("TriggerStatus", judith_trigger_status, "HitTriggerStatus[NHits]/b");
	hits_ttree->Branch("EventStatus", judith_event_status, "HitEventStatus[NHits]/s");


	// Judith event
	Long64_t n_entries_events = 0;
	if (fill_event) {
		event_ttree->Branch("TimeStamp", &judith_time_stamp, "TimeStamp/l");
		event_ttree->Branch("FrameNumber", &judith_frame_number, "FrameNumber/l");
		event_ttree->Branch("TriggerOffset", &judith_trigger_offset, "TriggerOffset/I");
		event_ttree->Branch("TriggerInfo", &judith_trigger_info, "TriggerInfo/I");
		event_ttree->Branch("Invalid", &judith_invalid, "Invalid/O");
	} else {
		event_ttree->SetBranchAddress("TimeStamp", &judith_time_stamp);
		event_ttree->SetBranchAddress("FrameNumber", &judith_frame_number);
		event_ttree->SetBranchAddress("TriggerOffset", &judith_trigger_offset);
		event_ttree->SetBranchAddress("TriggerInfo", &judith_trigger_info);
		event_ttree->SetBranchAddress("Invalid", &judith_invalid);
		n_entries_events = event_ttree->GetEntriesFast();
		if (n_entries_events == 0) {
			std::cout << "Event TTree is empty" << std::endl;
			throw;
		}
	}
	Long64_t n_entries = pybar_ttree->GetEntriesFast();

	for (Long64_t i = 0; i < n_entries; i++) {
		// entries will be overwritten in array
		pybar_ttree->GetEntry(i);
		// get array length
		Long64_t array_n_entries = (Long64_t) pybar_n_entries;
		std::cout << "reading chunk " << i+1 << " with size " << array_n_entries << std::endl;

		if (array_n_entries > BUF_SIZE) { // reached array limit
			std::cout << "reached max buffer size limit at chunk " << i+1 << std::endl;
			throw;
		}
		for (Long64_t j = 0; j < array_n_entries; j++) {
			// new event
			if (pybar_event_number[j] != curr_event_number) {
				// in case max_events is set and reached
				if (fill_event && max_events > 0 && event_counter >= max_events) {
					reached_max_events = true;
					std::cout << "reached max. events " << max_events << " at chunk " << i+1 << " index " << j << std::endl;
					break;
				}
				if (!fill_event && event_counter >= n_entries_events) {
					reached_max_events = true;
					std::cout << "reached max. events " << n_entries_events << " in " << plane << " at chunk " << i+1 << " index " << j << std::endl;
					break;
				}
				// read current
				current_time_stamp = pybar_trigger_time_stamp[j];
				curr_event_number = pybar_event_number[j];
				if (fill_event) { // add event to event TTree
					// fill event
					judith_time_stamp = (ULong64_t) current_time_stamp;
					judith_frame_number = (ULong64_t) curr_event_number;
					judith_trigger_offset = 0;
					judith_trigger_info = 0;
					// check for unknown words
					if ((pybar_event_status[j] & 0b0000000000010000) == 0b0000000000010000) {
						judith_invalid = 1;
					} else {
						judith_invalid = 0;
					}
					event_ttree->Fill();
				} else {
					Long64_t bytes = event_ttree->GetEntry(event_counter);
					if (bytes == 0) {
						std::cout << "invalid event in " << plane << " at chunk " << i+1 << " index " << j << std::endl;
						throw;
					}
					// check if event number of current plane is equal to event number in Event TTree
					if (judith_frame_number != (ULong64_t) curr_event_number) {
						std::cout << "event number mismatch in " << plane << " at chunk " << i+1 << " index " << j << std::endl;
						throw;
					}
					if (check_timestamp && event_counter>=2) {  // some files are missing the first trigger word, so check timestamp from 2nd trigger on
						if (judith_time_stamp >= judith_last_time_stamp) {
							judith_delta_time_stamp = (judith_time_stamp-judith_last_time_stamp);
						} else {
							judith_delta_time_stamp = judith_time_stamp+(pow(2, 31)-judith_last_time_stamp);
						}
						if (current_time_stamp >= last_time_stamp) {
							delta_time_stamp = (current_time_stamp-last_time_stamp);
						} else {
							delta_time_stamp = current_time_stamp+(pow(2, 31)-last_time_stamp);
						}
						time_stamp_ratio = (Double_t) judith_delta_time_stamp / (Double_t) delta_time_stamp;
						if (time_stamp_ratio < (1.0-pow(1, -2)) || time_stamp_ratio > (1.0+pow(1, -2))) {
							std::cout << "event timestamp mismatch in " << plane << " at chunk " << i+1 << " index " << j << std::endl;
							throw;
						}

					}
					judith_last_time_stamp = judith_time_stamp;
					last_time_stamp = current_time_stamp;
				}
				// fill hits TTree, take care of first event
				if (event_counter != 0) {
					hits_ttree->Fill();
				}
				// increment event counter
				event_counter++;
				// set hits array size to 0
				judith_n_hits = 0;
			}

			if ((pybar_column[j] == 0 || pybar_row[j] == 0) && judith_n_hits == 0) { // empty event, no hit
				;
			} else if (judith_n_hits >= MAX_HITS) { // reached array limit
				std::cout << "reached max hits limit at chunk " << i+1 << " index " << j << std::endl;
				throw;
			} else { // add hit to hit TTree
				// check for invalid column and row
				if (pybar_column[j] <= 0 || pybar_row[j] <= 0 || pybar_column[j] > 80 || pybar_row[j] > 336) {
					std::cout << "found invalid hit at chunk " << i+1 << " index " << j << std::endl;
					throw;
				}
				// pyBAR: starting col / row from 1
				// Judith: starting col / row from 0
				judith_hit_pix_x[judith_n_hits] = (Int_t) (pybar_column[j] - 1);
				judith_hit_pix_y[judith_n_hits] = (Int_t) (pybar_row[j] - 1);
				judith_hit_value[judith_n_hits] = (Int_t) pybar_tot[j];
				judith_hit_timing[judith_n_hits] = (Int_t) pybar_relative_bcid[j];
				judith_hit_in_cluster[judith_n_hits] = -1; // no cluster assigned
				judith_hit_pos_x[judith_n_hits] = 0.0; // not calculated
				judith_hit_pos_y[judith_n_hits] = 0.0; // not calculated
				judith_hit_pos_z[judith_n_hits] = 0.0; // not calculated
				judith_tdc[judith_n_hits] = (UShort_t) pybar_tdc[j];
				judith_tdc_time_stamp[judith_n_hits] = (UShort_t) pybar_tdc_time_stamp[j];
				judith_trigger_status[judith_n_hits] = (UChar_t) pybar_trigger_status[j];
				judith_event_status[judith_n_hits] = (UShort_t) pybar_event_status[j];

				judith_n_hits++;
			}
		}
		// check for max events
		if (reached_max_events) {
			break;
		}
	}
	// fill hits TTree with last event
	hits_ttree->Fill();
	// check for missing events
	if (!fill_event && event_counter < n_entries_events) {
		std::cout << "missing " << n_entries_events-event_counter << " events in " << plane << std::endl;
		throw;
	}

	i_file->Close();
	delete i_file;
	j_file->Write(0, TObject::kOverwrite);
	j_file->Close();
	delete j_file;

	return event_counter;
}
