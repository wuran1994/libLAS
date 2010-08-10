#include "kdx_util.hpp"



KDXIndexSummary::KDXIndexSummary(std::istream& input) :  bounds(), m_first(true)
{
    long id_count = 0;
    long id = 0;
    long i = 0;

    
    double low[2];
    double high[2];
    
    double mins[2];
    double maxs[2];
    
    bool first = true;
    
    while(input) {
        input >> id >> id_count >> low[0] >> low[1] >> high[0] >> high[1];
        // printf("count:%d %.2f %.2f %.2f %.2f\n", id_count, low[0], low[1], high[0], high[1]);
        
        if (first) {
            mins[0] = low[0];
            mins[1] = low[1];
            maxs[0] = high[0];
            maxs[1] = high[1];
            first = false;
        }
        
        mins[0] = std::min(mins[0], low[0]);
        mins[1] = std::min(mins[1], low[1]);
        
        maxs[0] = std::max(maxs[0], high[0]);
        maxs[1] = std::max(maxs[1], high[1]);
        // if (!input.good()) continue;
        
        IDVector ids;
        for (int j=0; j<id_count; j++) {
            input >> i;
            ids.push_back(i);
        }
        liblas::Bounds<double> b(low[0], low[1], high[0],high[1]);
        // SpatialIndex::Region* pr = new SpatialIndex::Region(low, high, 2);
        // printf("Ids size: %d %.3f\n", ids.size(), pr->getLow(0));
        IndexResult result(static_cast<uint32_t>(id));
        result.SetIDs(ids);
        result.SetBounds(b);
        m_results.push_back(result);
    }

    bounds = boost::shared_ptr<liblas::Bounds<double > >(new liblas::Bounds<double>(mins[0], mins[1], maxs[0], maxs[1]));
}

bool GetPointData(  liblas::Point const& p, 
                    // bool bTime, 
                    std::vector<uint8_t>& point_data)
{
    // This function returns an array of bytes describing the 
    // x,y,z and optionally time values for the point.  

    point_data.clear();

    double x = p.GetX();
    double y = p.GetY();
    double z = p.GetZ();
    // double t = p.GetTime();

    uint8_t* x_b =  reinterpret_cast<uint8_t*>(&x);
    uint8_t* y_b =  reinterpret_cast<uint8_t*>(&y);
    uint8_t* z_b =  reinterpret_cast<uint8_t*>(&z);

    // liblas::uint8_t* t_b =  reinterpret_cast<liblas::uint8_t*>(&t);

    // doubles are 8 bytes long.  For each double, push back the 
    // byte.  We do this for all four values (x,y,z,t)

    // // little-endian
    // for (int i=0; i<sizeof(double); i++) {
    //     point_data.push_back(y_b[i]);
    // }
    // 

    // big-endian
    for (int i = sizeof(double) - 1; i >= 0; i--) {
        point_data.push_back(x_b[i]);
    }

    for (int i = sizeof(double) - 1; i >= 0; i--) {
        point_data.push_back(y_b[i]);
    }   

    for (int i = sizeof(double) - 1; i >= 0; i--) {
        point_data.push_back(z_b[i]);
    }

    
    // if (bTime)
    // {
    //     for (int i = sizeof(double) - 1; i >= 0; i--) {
    //         point_data.push_back(t_b[i]);
    //     }
    // 
    // }

    return true;
}

void IndexResult::GetData(liblas::Reader* reader, std::vector<uint8_t>& data)
{
    IDVector const& ids = GetIDs();


    // d 8-byte IEEE  big-endian doubles, where d is the PC_TOT_DIMENSIONS value
    // 4-byte big-endian integer for the BLK_ID value
    // 4-byte big-endian integer for the PT_ID value
    
    
    data.clear(); 
    
    IDVector::const_iterator i;
    std::vector<uint8_t>::iterator pi;
    
    uint32_t block_id = GetID();

    std::vector<uint8_t> point_data;
    
    for (i=ids.begin(); i!=ids.end(); ++i) 
    {
        uint32_t id = *i;

        bool doRead = reader->ReadPointAt(id);
        if (doRead) {
            liblas::Point const& p = reader->GetPoint();

            // d 8-byte IEEE  big-endian doubles, where d is the PC_TOT_DIMENSIONS value
            
            
            bool gotdata = GetPointData(p, point_data);
            
            if (!gotdata) { throw std::runtime_error("Unable to fetch Point Data"); exit(1);}
            std::vector<uint8_t>::const_iterator d;
            for (d = point_data.begin(); d!=point_data.end(); ++d) {
                data.push_back(*d);
            }

            uint8_t* id_b = reinterpret_cast<uint8_t*>(&id);
            uint8_t* block_b = reinterpret_cast<uint8_t*>(&block_id);
            
            // 4-byte big-endian integer for the BLK_ID value
            for (int i =  sizeof(uint32_t) - 1; i >= 0; i--) {
                data.push_back(block_b[i]);
            }
            
            // 4-byte big-endian integer for the PT_ID value
            for (int i =  sizeof(uint32_t) - 1; i >= 0; i--) {
                data.push_back(id_b[i]);
            }
            

        }
    }

}


bool KDTreeIndexExists(std::string& filename)
{
    struct stat stats;
    std::ostringstream os;
    os << filename << ".kdx";

    std::string indexname = os.str();
    
    // ret is -1 for no file existing and 0 for existing
    int ret = stat(indexname.c_str(),&stats);

    bool output = false;
    if (ret == 0) output= true; else output =false;
    return output;
}

                    
