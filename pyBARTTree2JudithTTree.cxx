/* Converter from pyBAR TTree to Judith TTree.
 * Compatible with CERN ROOT 5 and 6.
 */

#include <iostream>
#include <algorithm>

#include <TFile.h>
#include <TTree.h>
#include <TBranch.h>
#include <TDirectory.h>


int pyBARTTree2JudithTTree(const char* input_file_name, const char* output_file_name, const char* plane = "Plane0", const char* mode = "RECREATE", bool fill_event = true, Long64_t max_events = 0) {
	// Call with: root -l -b -q "pyBARTTree2JudithTTree.cxx(\"input.root\", \"output.root\", \"Plane0\", \"RECREATE\", 1, 0))"
	// To add another plane: root -l -b -q "pyBARTTree2JudithTTree.cxx(\"input.root\", \"output.root\", \"Plane1\", \"UPDATE\", 0, 0))"

	// the buffer size depends on the input file
	static const Long64_t BUF_SIZE = 100000;
	static const Int_t MAX_HITS = 4000;

	// pyBAR ROOT file
	TFile* i_file = new TFile(input_file_name);
	TTree* pybar_table = (TTree*) i_file->Get("Hits");

	// Judith ROOT file
	TFile* j_file = new TFile(output_file_name, mode);
	TTree* event;
	if (fill_event) {
		event = (TTree*) j_file->Get("Event");
		if (event != 0) {
			std::cout << "Event TTree already exists" << std::endl;
			throw;
		}
		event = new TTree("Event", "Event");
	} else {
		event = (TTree*) j_file->Get("Event");
		if (event == 0) {
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
	TTree* hits = new TTree("Hits", "Hits");

	// event number and counter
	Long64_t event_counter = 0;
	Long64_t curr_event_number = -1; // set to invalid event number
	bool reached_max_events = false;

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
	UChar_t pybar_tdc_time_stamp[BUF_SIZE];
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

	// Event arrays
	ULong64_t judith_time_stamp;
	ULong64_t judith_frame_number;
	Int_t judith_trigger_offset;
	Int_t judith_trigger_info;
	Bool_t judith_invalid;

	// pyBAR branches
	pybar_table->SetBranchAddress("n_entries", &pybar_n_entries);
	pybar_table->SetBranchAddress("event_number", pybar_event_number);
	pybar_table->SetBranchAddress("trigger_number", pybar_trigger_number);
	pybar_table->SetBranchAddress("trigger_time_stamp", pybar_trigger_time_stamp);
	pybar_table->SetBranchAddress("relative_BCID", pybar_relative_bcid);
	pybar_table->SetBranchAddress("LVL1ID", pybar_lvl1id);
	pybar_table->SetBranchAddress("column", pybar_column);
	pybar_table->SetBranchAddress("row", pybar_row);
	pybar_table->SetBranchAddress("tot", pybar_tot);
	pybar_table->SetBranchAddress("BCID", pybar_bcid);
	pybar_table->SetBranchAddress("TDC", pybar_tdc);
	pybar_table->SetBranchAddress("TDC_time_stamp", pybar_tdc_time_stamp);
	pybar_table->SetBranchAddress("trigger_status", pybar_trigger_status);
	pybar_table->SetBranchAddress("service_record", pybar_service_record);
	pybar_table->SetBranchAddress("event_status", pybar_event_status);

	// Judith branches
	hits->Branch("NHits", &judith_n_hits, "NHits/I");
	hits->Branch("PixX", judith_hit_pix_x, "HitPixX[NHits]/I");
	hits->Branch("PixY", judith_hit_pix_y, "HitPixY[NHits]/I");
	hits->Branch("Value", judith_hit_value, "HitValue[NHits]/I");
	hits->Branch("Timing", judith_hit_timing, "HitTiming[NHits]/I");
	hits->Branch("InCluster", judith_hit_in_cluster, "HitInCluster[NHits]/I");
	hits->Branch("PosX", judith_hit_pos_x, "HitPosX[NHits]/D");
	hits->Branch("PosY", judith_hit_pos_y, "HitPosY[NHits]/D");
	hits->Branch("PosZ", judith_hit_pos_z, "HitPosZ[NHits]/D");

	// Judith event
	Long64_t n_entries_events = 0;
	if (fill_event) {
		event->Branch("TimeStamp", &judith_time_stamp, "TimeStamp/l");
		event->Branch("FrameNumber", &judith_frame_number, "FrameNumber/l");
		event->Branch("TriggerOffset", &judith_trigger_offset, "TriggerOffset/I");
		event->Branch("TriggerInfo", &judith_trigger_info, "TriggerInfo/I");
		event->Branch("Invalid", &judith_invalid, "Invalid/O");
	} else {
		event->SetBranchAddress("TimeStamp", &judith_time_stamp);
		event->SetBranchAddress("FrameNumber", &judith_frame_number);
		event->SetBranchAddress("TriggerOffset", &judith_trigger_offset);
		event->SetBranchAddress("TriggerInfo", &judith_trigger_info);
		event->SetBranchAddress("Invalid", &judith_invalid);
		n_entries_events = event->GetEntriesFast();
		if (n_entries_events == 0) {
			std::cout << "Event TTree is empty" << std::endl;
			throw;
		}
	}
	Long64_t n_entries = pybar_table->GetEntriesFast();

	for (Long64_t i = 0; i < n_entries; i++) {
		// entries will be overwritten in array
		pybar_table->GetEntry(i);
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
				curr_event_number = pybar_event_number[j];
				if (fill_event) { // add event to event TTree
					// fill event
					judith_time_stamp = (ULong64_t) pybar_trigger_time_stamp[j];
					judith_frame_number = (ULong64_t) curr_event_number;
					judith_trigger_offset = 0;
					judith_trigger_info = 0;
					// check for unknown words
					if ((pybar_event_status[j] & 0b0000000000010000) == 0b0000000000010000) {
						judith_invalid = 1;
					} else {
						judith_invalid = 0;
					}
					event->Fill();
				} else {
					Long64_t bytes = event->GetEntry(event_counter);
					if (bytes == 0) {
						std::cout << "invalid event in " << plane << " at chunk " << i+1 << " index " << j << std::endl;
						throw;
					}
					// check if event number of current plane is equal to event number in Event TTree
					if (judith_frame_number != (ULong64_t) curr_event_number) {
						std::cout << "event number mismatch in " << plane << " at chunk " << i+1 << " index " << j << std::endl;
						throw;
					}
				}
				// fill hits TTree, take care of first event
				if (event_counter != 0) {
					hits->Fill();
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
				judith_n_hits++;
			}
		}
		// check for max events
		if (reached_max_events) {
			break;
		}
	}
	// fill hits TTree of last event
	hits->Fill();
	// check for missing events
	if (!fill_event && event_counter < n_entries_events) {
		std::cout << "missing " << n_entries_events-event_counter << " events in " << plane << std::endl;
		throw;
	}

	j_file->Write();
	j_file->Close();

	return event_counter;
}
